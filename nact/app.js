const { start, dispatch, stop, spawnStateless, spawn } = require('nact');
const system = start();
const fs = require('fs');
//const data = JSON.parse(fs.readFileSync('data.json'));
//const data = JSON.parse(fs.readFileSync('IFF87_SirvydasS_d2.json'));
const data = JSON.parse(fs.readFileSync('IFF87_SirvydasS_d3.json'));
const companies = data.Companies;

//debbuginimui, kad lengviau rastumeme kur koks dydis atkeliauja.
const delay = (time) => new Promise((res) => setTimeout(res, time));

//duomenu dydis
const DATASIZE = 26;


//parenkama kiek bus darbiniu aktoriu
const darb_aktoriu = 6;


//zinuiu tipai, kuriais apsikeitineja aktoriai.
const types = {
  SEND_TO_WORKER:"SEND_TO_WORKER",
  SEND_TO_RESULTS:"SEND_TO_RESULTS",
  LAST_TO_CHECK:"LAST_TO_CHECK",
  GET_RESULTS: "GET_RESULTS",
  SEND_TO_PRINT:"SEND_TO_PRINT",
  RESULTS_SIZE:"RESULTS_SIZE",
  THROW_AWAY:"THROW_AWAY",
  FINISHED:"FINISHED",
  SUCCESS: "SUCCESS",
};



//pagrindinis aktorius- skirstytuvas, skirsto zinutes tarp kitu aktoriu,
//bet pats nieko neskaiciuoja.
const skirstytuvas = spawn(
  system, //tevas
  (state={ kazkas:{sent_to_R:0, resultsSize:0}},msg,ctx)=>{ //saugo busena kiek jau issiunte darbuotojams ir rezultatu kaupikliui
    const type = msg.type; //gaunamos zinutes tipas
    if(type === types.SEND_TO_WORKER) // sis tipas ateina kai pagr. scenarijus atsiuncia duomenis
    {
      const item = msg.item;
      const index = msg.ind;
      const nextState = {
        kazkas:{
          ...state.kazkas,['sent_to_W']: index,
        },
      };
      if(nextState.kazkas['sent_to_W'] === DATASIZE - 1){ //jei tai jau paskutinis pranesimas
        dispatch(darbuotojasTarpininkas, {type:types.LAST_TO_CHECK, name:item.name, empl:item.employees, avgSal:item.avgSalary, ind:index})
        dispatch(spausdintojas, {type:types.LAST_TO_CHECK, name:item.name, empl:item.employees, avgSal:item.avgSalary, ind:index})
      }
      else { //jei nepaskutinis tiesiog siunciam reguliaru pranesima darbuotojui, kad apdorotu ir suskaiciuotu.
        dispatch(darbuotojasTarpininkas, {type:type, name:item.name, empl:item.employees, avgSal:item.avgSalary, ind:index});
        dispatch(spausdintojas, {type:type, name:item.name, empl:item.employees, avgSal:item.avgSalary, ind:index}); // inputui irasineti siunciam ir i spausdintoja viena kopija.
      }
      return nextState;
    }
    if(type === types.SEND_TO_RESULTS){ //tipas kai apskaiciuota suma atitinka kriteriju.
      const nextState = {
        kazkas:{
          ...state.kazkas,['sent_to_R']: (state.kazkas['sent_to_R'] + 1),
        },
      };
      dispatch(rezultatuKaupiklis, {msg, index:state.kazkas['sent_to_R']});
      console.log(state);
      return nextState;
    }
    if(type === types.RESULTS_SIZE){//paskutinis pranesimas rezultatu kaupikliui, kuris signalizuoja jog reikia atsiust jau rezultatus
      const nextState = {
        kazkas:{
          ...state.kazkas,['resultsSize']: msg.size,
        },
      };
      dispatch(rezultatuKaupiklis, {type:types.RESULTS_SIZE, size:msg.size});
      return nextState;
    }
    if(type === types.SEND_TO_PRINT){ //tipas kai zinute parejusi is rezultatu kaupiklio sako, jog gautus duomenis siusti reikia i spausdintoja
      dispatch(spausdintojas,{type:type, array:msg.array});
    }
    if(type=== types.FINISHED){ //kai viskas baigta spausdinti gaunama zinute, jog stabdome aktoriu sistema.
      stop(system);
    }
  },
  'Skirstytuvas'
);

//spausdintojas spausdina gaunamus duomenis i rezultatu faila.
const spausdintojas = spawn(
  skirstytuvas,
  (state={},msg,ctx) =>{
    const writeInput = state['input'];
    console.log(`writeInput=${writeInput}`);
    if(!writeInput){ //jei dar nerase input zodzio, tai istrina kas buvo pries tai ir iraso zodi "INPUT" i faila
      fs.writeFile('./rez.txt',"==============INPUT================ \n", (err) =>{
        if(err) throw err;
      })
    }
    const type = msg.type;
    if(type===types.SEND_TO_WORKER){ // Jei zinute buvo siunciama ir darbuotojui, suprantam, jog cia inputas, ir rasom prie input.
      fs.appendFile('./rez.txt',`${msg.name}\t\t${msg.empl}\t\t\t${msg.avgSal}` + "\n",(err)=>{
        if(err) throw err;
      });
    }
    if(type ===types.SEND_TO_PRINT){ // sio tipo zinute ateina, kai gauname rezultatus ir zinome jog reikia iterpti jau rezultatu masyva i OUTPUT.
     // console.log(msg.array);
      const companies = msg.array;
      fs.appendFile('./rez.txt', "===============OUTPUT================\n",(err) =>{
        if(err) console.log("CIA klaida jau vyksta")//throw err;
      });


      //kvieciame foreach metoda kiekvienam rezultato bjektui.
      Object.keys(companies).forEach(key =>{
        fs.appendFile('./rez.txt',`${companies[key].name}\t\t${companies[key].empl}\t\t\t${companies[key].avgSal}\t\t\t${companies[key].sum}` + "\n",(err)=>{
          if(err) throw err;
        });
      });
      //isiunciame zinute tevui, signalizuoja, jog darbas baigtas, galima isjungti aktoriu sistema ir pati baigia darba
      dispatch(ctx.parent, {type:types.FINISHED});
      stop(spausdintojas);
    }
    return { ...state, ['input']: true };
  },
  'Spausdintojas'
)

//Rezultatu kaupiklis- kaupia rezultatus ir visis juos surinkes persiuncia atgal skirstytojui.
const rezultatuKaupiklis = spawn(
  skirstytuvas,
  async(state={array:{}},msg,ctx)=>
  {
      const type = msg.type;
      if(type === types.RESULTS_SIZE) // jei ateina zinute su rezultatu dydziu reikia isiusti zinute skirstytojui su visais rezultatais
      {
        console.log("===================>Kazka cia");
        dispatch(ctx.parent, {array:state.array, type:types.SEND_TO_PRINT});
        stop(rezultatuKaupiklis);
      }
      else  { //kitu atveju kaupiame rezultatus busenos masyve.
        const item = msg.msg;
        const cur = state.array;
        const object = {"name":item.name, "empl":item.empl, "avgSal":item.avgSal, "sum":item.sum};
        console.log("REZULATAI===============================");
        const whereTo = Object.keys(cur).filter(key => cur[key].sum > object.sum);
        const whereTo2 = whereTo[whereTo.length-1];
        const index = msg.index;
//        const index = whereTo2 == undefined ? msg.index :whereTo2;
        const nextState = {
          array:{
            ...state.array,[index]:object
          },
        };
        return nextState;
    }
  },'Kaupiklis'
) 

//darbuotojas tarpininkas, kuris gauna duomenis ir sukuria tiek darbininku kiek nurodyta
//tada duoda jiems duomenis, pagal tai kokio atkeliavo indekso objektas.

const darbuotojasTarpininkas = spawn(
  skirstytuvas,
  async (state={ proccessed:{iterations:0, resultsSize:0} },msg,ctx) => {
    const msgType= msg.type;
    if(msgType === types.SEND_TO_WORKER || msgType === types.LAST_TO_CHECK){
      const toWho = (msg.ind % darb_aktoriu).toString();
      let childActor;
      //jei jau yra sukurtas toks aktorius su tokiu ID
      if(ctx.children.has(toWho))
      {
        childActor = ctx.children.get(toWho);
      }
      else{ //jei nera, sukuriamas aktorius
        childActor = darbuotojasVaikas(ctx.self, toWho);    
      }
      dispatch(childActor, msg);
    }

    if(msgType === types.SEND_TO_RESULTS){ //griztamasis rysys ir darbininko, kad tinka, ir siunciam i rezultatu kaupikli gauta objekta.
      dispatch(ctx.parent, msg);
      const nextState = {
        proccessed:{
          ...state.proccessed,['resultsSize']: (state.proccessed['resultsSize'] + 1),
        },
        
      };
      // jei padarytu iteraciju ir rezultatu issiustu skaicius lygus duomenu dydziui issiunciam rezultatu dydi ir zinute kad jau galima imti rezultatus.
      if(nextState.proccessed['resultsSize'] + state.proccessed['iterations'] === DATASIZE){
        dispatch(ctx.parent, {type:types.RESULTS_SIZE, size:nextState.proccessed['resultsSize']});
        stop(darbuotojasTarpininkas);
      }
      console.log(`Siunciame uzklausa skirstytuvui, kad talpintu i masyva ${msg.name}. ${nextState.proccessed['iterations']}`);
      return nextState;    
    }

    if(msgType === types.THROW_AWAY){ //zinute jog netiko objektas, per maza saknis ir priskaiciuojame tik dar viena iteracija
      const nextState = {
        proccessed:{
          ...state.proccessed,['iterations']: (state.proccessed['iterations'] + 1),
        },
      };
      if(nextState.proccessed['iterations'] + nextState.proccessed['resultsSize'] === DATASIZE){ // jei jau visi duomenys apdoroti
        dispatch(ctx.parent, {type:types.RESULTS_SIZE, size:state.proccessed['resultsSize']});
        stop(darbuotojasTarpininkas);
      }
    return nextState;    
    }
  }
)

//sukurtas vaikas darbuotojas apdoroja duomenis ir siuncia juos atgal darbuotoju skirstytojui/tarpininkui
const darbuotojasVaikas = (parent, workerId) => spawn(
  parent,
  async(state = {},msg,ctx)=>{
    console.log(`Darbuotojas vaikas:${ctx.self.name} Tikrina ${msg.name}`);
    await delay(500);
    const Sum = Math.sqrt(msg.empl * msg.avgSal);
    if(Sum>300)
    {
      dispatch(ctx.parent, {name:msg.name, empl:msg.empl, avgSal:msg.avgSal, sum:Sum, type:types.SEND_TO_RESULTS});
    } else{
      dispatch(ctx.parent, {type:types.THROW_AWAY});
    }
  },
  workerId
)

//pagrindinis scenarijus kuris issiuntineja visus duomenis.
  companies.forEach((item, index) =>  {
    dispatch(skirstytuvas,{item:item, ind:index, type:types.SEND_TO_WORKER});
  });

 
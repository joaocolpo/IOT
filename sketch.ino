#include <WiFi.h>
#include <WebServer.h>
#include <DHT.h>
#define DHT_PIN 15
#define PIR_PIN 4
#define LED_MOV 18
#define LED_CLIMA 19
#define BUZZER_PIN 5
#define DHT_TYPE DHT22
const char* SSID="Wokwi-GUEST";
const char* PASSWORD="";
const float LIGAR_C=27.0f;
const float DESLIGAR_C=24.0f;
const unsigned long TEMPO_SEM_MOVIMENTO=30000UL; // 30 s para demonstracao; em uso real: 15 min
const unsigned long INTERVALO=2000UL;
const float POTENCIA_ESTIMADA_KW=1.2f; // hipotese ilustrativa, nao medicao
DHT dht(DHT_PIN,DHT_TYPE);
WebServer server(80);
float temp=NAN,hum=NAN;
bool sensorOK=false,movimento=false,ocupada=false,clima=false,alerta=false;
bool modoAutomatico=true,manualLigado=false;
unsigned long ultimoMovimento=0,ultimaLeitura=0,ultimoRelogio=0,ultimoWifi=0;
unsigned long tempoOcupadaMs=0,tempoClimaMs=0,entradas=0;
const char PAGINA[] PROGMEM=R"HTML(<!DOCTYPE html><html lang="pt-BR"><head><meta charset="UTF-8"><meta name="viewport" content="width=device-width,initial-scale=1"><title>Climatizacao IoT</title><style>body{font-family:Arial;background:#edf3f9;color:#203044;margin:0}header{background:#163b68;color:white;padding:22px;text-align:center}main{max-width:950px;margin:20px auto;padding:12px}.grid{display:grid;grid-template-columns:repeat(auto-fit,minmax(180px,1fr));gap:12px}.card{background:white;padding:18px;border-radius:12px;box-shadow:0 2px 10px #0001;margin-bottom:12px}.big{font-size:27px;font-weight:bold}button{padding:12px;margin:5px;border:0;border-radius:7px;background:#174d86;color:white;cursor:pointer}pre{white-space:pre-wrap;overflow-wrap:anywhere}</style></head><body><header><h1>Sala universitaria - monitoramento e climatizacao</h1></header><main><div class="grid"><div class="card">Temperatura<div class="big" id="temp">--</div></div><div class="card">Umidade<div class="big" id="hum">--</div></div><div class="card">Movimento<div class="big" id="mov">--</div></div><div class="card">Sala em uso (estimada)<div class="big" id="ocup">--</div></div><div class="card">Climatizacao simulada<div class="big" id="clima">--</div></div><div class="card">Modo<div class="big" id="modo">--</div></div></div><div class="card"><h2>Controle</h2><button onclick="cmd('auto')">Automatico</button><button onclick="cmd('on')">Ligar manualmente</button><button onclick="cmd('off')">Desligar manualmente</button><p id="status">Aguardando</p><p>O LED vermelho representa o comando de climatizacao; nao ha ar-condicionado real conectado.</p></div><div class="card"><h2>Indicadores acumulados desde a inicializacao</h2><p id="historico"></p><p>Energia e uma estimativa calculada, nao uma medicao.</p></div><div class="card"><h2>Payload JSON</h2><pre id="json">--</pre></div></main><script>async function atualizar(){try{let r=await fetch('/api');if(!r.ok)throw Error(r.status);let d=await r.json();document.getElementById('temp').textContent=d.temperatura_c===null?'indisponivel':d.temperatura_c.toFixed(1)+' °C';document.getElementById('hum').textContent=d.umidade_pct===null?'indisponivel':d.umidade_pct.toFixed(1)+' %';document.getElementById('mov').textContent=d.movimento?'Sim':'Nao';document.getElementById('ocup').textContent=d.sala_em_uso?'Sim':'Nao';document.getElementById('clima').textContent=d.climatizacao_ligada?'LIGADA':'DESLIGADA';document.getElementById('modo').textContent=d.modo;document.getElementById('historico').textContent='Deteccoes iniciais de uso: '+d.entradas_estimadas+' | Tempo em uso: '+d.tempo_ocupada_s+' s | Climatizacao: '+d.tempo_climatizacao_s+' s | Energia estimada: '+d.energia_estimada_kwh.toFixed(4)+' kWh';document.getElementById('json').textContent=JSON.stringify(d,null,2);document.getElementById('status').textContent='Dados atualizados';}catch(e){document.getElementById('status').textContent='Sem comunicacao: '+e.message}}async function cmd(x){try{let r=await fetch('/api/modo',{method:'POST',headers:{'Content-Type':'text/plain'},body:x});if(!r.ok)throw Error('Comando invalido');atualizar()}catch(e){alert(e.message)}}atualizar();setInterval(atualizar,2000)</script></body></html>)HTML";
void aplicar(){
 if(modoAutomatico){
   if(!ocupada || !sensorOK) clima=false;
   else if(!clima && temp>=LIGAR_C) clima=true;
   else if(clima && temp<=DESLIGAR_C) clima=false;
 }else clima=manualLigado;
 alerta=!sensorOK;
 digitalWrite(LED_MOV,movimento?HIGH:LOW);
 digitalWrite(LED_CLIMA,clima?HIGH:LOW);
 digitalWrite(BUZZER_PIN,alerta?HIGH:LOW);
}
String payload();
void ler(){
 movimento=digitalRead(PIR_PIN)==HIGH;
 bool anterior=ocupada;
 if(movimento){ultimoMovimento=millis();ocupada=true;}
 else ocupada=ultimoMovimento!=0 && millis()-ultimoMovimento<TEMPO_SEM_MOVIMENTO;
 if(!anterior && ocupada) entradas++;
 float t=dht.readTemperature(),h=dht.readHumidity();
 sensorOK=!isnan(t)&&!isnan(h);
 if(sensorOK){temp=t;hum=h;}
 aplicar();
 Serial.println("==========================");
 Serial.print("Temperatura: "); if(sensorOK){Serial.print(temp,1);Serial.println(" C");}else Serial.println("Indisponivel");
 Serial.print("Umidade: "); if(sensorOK){Serial.print(hum,1);Serial.println(" %");}else Serial.println("Indisponivel");
 Serial.print("Movimento: "); Serial.println(movimento?"Detectado":"Nao detectado");
 Serial.print("Sala: "); Serial.println(ocupada?"Ocupada":"Desocupada");
 Serial.print("Sensor DHT22: "); Serial.println(sensorOK?"Funcionando":"Falha");
 Serial.print("Ar-condicionado: "); Serial.println(clima?"Ligado":"Desligado");
 Serial.print("JSON: "); Serial.println(payload());
 Serial.println("==========================");
}
String payload(){
 String s="{\"dispositivo\":\"sala-01\",\"temperatura_c\":";
 s+=sensorOK?String(temp,1):"null";s+=",\"umidade_pct\":";
 s+=sensorOK?String(hum,1):"null";
 s+=",\"movimento\":"+String(movimento?"true":"false");
 s+=",\"sala_em_uso\":"+String(ocupada?"true":"false");
 s+=",\"climatizacao_ligada\":"+String(clima?"true":"false");
 s+=",\"modo\":\""+String(modoAutomatico?"automatico":"manual")+"\"";
 s+=",\"sensor_valido\":"+String(sensorOK?"true":"false");
 s+=",\"entradas_estimadas\":"+String(entradas);
 s+=",\"tempo_ocupada_s\":"+String(tempoOcupadaMs/1000);
 s+=",\"tempo_climatizacao_s\":"+String(tempoClimaMs/1000);
 s+=",\"energia_estimada_kwh\":"+String(POTENCIA_ESTIMADA_KW*tempoClimaMs/3600000.0f,4);
 s+=",\"tempo_ms\":"+String(millis())+"}";
 return s;
}
void comando(){String x=server.arg("plain");x.trim();x.toLowerCase();
 if(x=="auto")modoAutomatico=true;
 else if(x=="on"){modoAutomatico=false;manualLigado=true;}
 else if(x=="off"){modoAutomatico=false;manualLigado=false;}
 else{server.send(400,"application/json","{\"erro\":\"use auto, on ou off\"}");return;}
 aplicar();server.send(200,"application/json",payload());}
void setup(){Serial.begin(115200);pinMode(PIR_PIN,INPUT);pinMode(LED_MOV,OUTPUT);pinMode(LED_CLIMA,OUTPUT);pinMode(BUZZER_PIN,OUTPUT);dht.begin();aplicar();
 WiFi.mode(WIFI_STA);WiFi.begin(SSID,PASSWORD,6);
 server.on("/",HTTP_GET,[](){server.send_P(200,"text/html; charset=utf-8",PAGINA);});
 server.on("/api",HTTP_GET,[](){server.send(200,"application/json",payload());});
 server.on("/api/modo",HTTP_POST,comando);
 server.begin();Serial.println("Sistema iniciado. Verifique o acesso HTTP na porta 80 do Wokwi.");}
void loop(){unsigned long agora=millis();unsigned long delta=agora-ultimoRelogio;ultimoRelogio=agora;
 if(ocupada)tempoOcupadaMs+=delta;if(clima)tempoClimaMs+=delta;
 if(agora-ultimaLeitura>=INTERVALO){ultimaLeitura=agora;ler();}
 if(WiFi.status()==WL_CONNECTED)server.handleClient();
 else if(agora-ultimoWifi>=10000){ultimoWifi=agora;WiFi.reconnect();}
}

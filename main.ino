#include <ESP8266WiFi.h>            // Biblioteca do modulo ESP8266

// Definição de parametros do WiFi ////////////
#define WLAN_SSID       "name"        // WiFi name
#define WLAN_PASS       "password"    // WiFi password
/////////////////////////////////////////////// 

// Métodos de acesso à internet //////////////
WiFiClientSecure client;            //Cria um cliente seguro (para ter acesso ao HTTPS)
String linkDataBase = "GET /forms/d/e/1FAIpQLScmXKASf1EhI6jFGrbKF85sObhdYVpwp3fvu6q18bTpWK5sew/formResponse?ifq&";
String linkUmidade = "entry.995652966=";
String linkReservatorio = "&entry.1111876016=";
String linkValvula = "&entry.1475558957=";
boolean sendData;                       // Variavel de controle do metodo de atualizacao do banco de dados
//Strings auxiliares com o link do formulario do database utilizado pelo GET e das caixas de resposta do formulario
///////////////////////////////////////////////

// Soil Mosture Sensor (SMS) //////////////////
#define SMS A0                      // ESP8266 pin
double SMS_Value = 0.0;
double SMS_Volts = 0.0;
double SMS_Perct = 0.0;
double WLS_Cdt = 0.0;               // Water Level Sensor Condition
#define SMS_Dry 3.08                // Parametros para conversao do valor lido em porcentagem
#define SMS_Moist 1.2
#define SMS_Perct_Min 40            // Parametro minimo de umidade (controle da valvula)      
///////////////////////////////////////////////

// Water Level Sensor (WLS) ///////////////////
//#define WLS D2                      // Water Level Sensor pin
String levelState = "Bom";          // Estado do nivel do reservatorio
///////////////////////////////////////////////

// Valve //////////////////////////////////////
#define VALVE D0                    // Valve pin
#define GLED D1                     // Led pin
String valveState = "Fechada";      // Estado da valvula
///////////////////////////////////////////////

void setup() {

  // Serial Monitor Setup /////////////////////
  Serial.begin(115200);
  delay(10);
  ////////////////////////////////////////////

  // WiFi Setup /////////////////////////////
  Serial.println("Connecting to WiFi (");     //Mostra no monitor serial a rede WiFi que irá se conectar
  Serial.print(WLAN_SSID);
  Serial.print(", ");
  Serial.print(WLAN_PASS);
  Serial.print(") ");
  
  WiFi.begin(WLAN_SSID, WLAN_PASS);           //Conecta na rede WiFi
  while (WiFi.status() != WL_CONNECTED) {     //Loop até a conexão com a rede
    delay(500);
    Serial.print(".");
  }
  
  Serial.println("");
  Serial.println("Connected to WiFi");
  ////////////////////////////////////////////

  /*Water Level Sensor //////////////////////
  pinMode (WLS,  INPUT);
  pinMode (RLED, OUTPUT);
  pinMode (GLED, OUTPUT);
  //////////////////////////////////////////*/

  //Valve ///////////////////////////////////
  pinMode (VALVE, OUTPUT);
  pinMode (GLED, OUTPUT);
  ///////////////////////////////////////////
}

void loop () {
    
  // Soil Mosture Sensor Iteration //////////
  SMS_Value = analogRead(SMS);                // Le o sinal analogico do SMS
  Serial.print("Soil Mosture Analog Value:"); // Printa valor no monitor serial
  Serial.println(SMS_Value);
  // convert the analog signal to voltage
  // the ESP2866 A0 reads between 0 and ~3 volts, producing a corresponding value
  // between 0 and 1024. The equation below will convert the value to a voltage value.
  SMS_Volts = (SMS_Value * 3.08) / 1024;
  Serial.print("Soil Mosture Analog Voltage:");
  Serial.println(SMS_Volts);

  int SMS_Perct_Aux = (1 - ((SMS_Volts-SMS_Moist)/(SMS_Dry-SMS_Moist)))*100;
  
  if (abs(SMS_Perct - SMS_Perct_Aux) >= 5.0) {          // Se a diferenca entre a leitura atual e a anterior for maior que 5
    SMS_Perct = SMS_Perct_Aux;                          // Atualizamos a leitura
    sendData = true;                                        // Hablitamos o envio ao banco de dados
  }
  ///////////////////////////////////////////


 /* Water Level Sensor Iteration ////////////
  int WLS_Cdt_Aux = digitalRead(WLS);           // Le o sinal digital
  Serial.print("Water Level Sensor Signal:");   // Printa no monitor serial a leitura
  Serial.println(WLS_Cdt_Aux);

  if (WLS_Cdt_Aux != WLS_Cdt)
    WLS_Cdt = WLS_Cdt_Aux;
  if(WLS_Cdt){                    // Se a variavel e diferente de zero
    analogWrite(GLED, LOW);      // Desligamos led verde
    analogWrite(RLED, HIGH);     // Ligamos o vermelho
    levelState = "Baixo";
  }
  else{ 
    analogWrite(GLED, HIGH);
    analogWrite(RLED, LOW);
    levelState = "Bom";
   }
 /////////////////////////////////////////*/

 // Valve Iteration ////////////////////////
  if (SMS_Perct < SMS_Perct_Min) {  // Se leitura de umidade for inferior ao valor mínimo
    digitalWrite(VALVE, HIGH);      // Abrimos a valvula
    digitalWrite(GLED, HIGH);       // Ligamos o led 
    Serial.println("Valve Open");   // Printamos no monitor serial
    valveState = "Aberta";
  }
  else {
    digitalWrite(VALVE, LOW);
    Serial.println("Valve Closed");
    digitalWrite(GLED, LOW); 
    valveState = "Fechada";
  }
 ///////////////////////////////////////////

// Database Iteration //////////////////////
  if(sendData == true) { 
    if (client.connect("docs.google.com", 443) == 1) {  // Tenta se conectar ao servidor do Google docs na porta 443 (HTTPS)
      String toSend = linkDataBase;                        // Atribuimos a String auxiliar na nova String que sera enviada
      toSend += linkUmidade;                          // Adicionamos a caixa de resposta da umidade do formulario
      toSend += SMS_Perct;                            // E adicionamos o valor de leitura da umidade
      toSend += linkReservatorio;                     // O mesmo para o nivel do reservatorio -
      toSend += levelState;                           // =
      toSend += linkValvula;                          // E para a valvula -
      toSend += valveState;                           // - 
      toSend += "&submit=Submit HTTP/1.1";            // Completamos o metodo GET para nosso formulario.
      client.println(toSend);                         // Enviamos o GET ao servidor-
      client.println("Host: docs.google.com");        // -
      client.println();                               // -
      client.stop();                                  // Encerramos a conexao com o servidor
      Serial.println("Dados enviados.");              // Mostra no monitor que foi enviado
      sendData = false;                                     // Setamos o valor de send para o seu valor inicial (indicando que publicamos)
    }
    else {
      Serial.println("Erro ao se conectar");          // Se nao for possivel conectar no servidor, ira avisar no monitor.
    }
  }
  ///////////////////////////////////////////

  delay(3000);  // Repete a cada 3s
}

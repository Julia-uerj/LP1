#include <SPI.h>
#include <MFRC522.h>
#include <EEPROM.h>
#include <string.h>
#include <LiquidCrystal_I2C.h>
#include <Wire.h>

LiquidCrystal_I2C lcd(0x3F, 2, 1, 0, 4, 5, 6, 7, 3, POSITIVE);

#define SS_PIN 10
#define RST_PIN 9
#define LED_R 2//LED Vermelho
#define LED_G 3 //LED Verde
#define TAM 2 //  QUANTIDADE DE USUARIOS
#define TAMUS 20 // TAMANHO DE BYTES DE CADA USUARIO
#define TAMSTR 16 // TAMANHO DA STRING DE CADA USUARIO
// CARTAO 57 BB 3F 0C
// TAG E2 9D 9D 2E


int posicao = 0;
int numpessoas = 0;

struct posmesa
{
  byte x;
  byte y;
};

struct usuario
{
  byte id; // identificacao da pessoa
  byte estado; // 1 - dentro da sala / 0 - fora da sala // estado armazenado na memoria flash - retirar do usuario - deixar somente na eeprom
  struct posmesa mesa;
  String tag;
  char nome[TAMSTR];
};

struct usuario users[TAM]; // guarda a struct dos usuarios a cada posicao do array

// prototipos de funcao
MFRC522 mfrc522(SS_PIN, RST_PIN);

void setup() 
{
  // put your setup code here, to run once:
  Serial.begin(9600);
  SPI.begin();
  lcd.begin(16,2);
  lcd.clear();
  // Inicia MFRC522    
  mfrc522.PCD_Init();
  Serial.println("Aproxime o seu cartao/TAG do leitor");
  Serial.println();
  pinMode(LED_R, 2);
  pinMode(LED_G, 3);

  // cria o prim usuario, na pos[0] do vetor, preenchendo nos campos: x = 50, y = 37, cartao  57 BB 3F 0C, nome "Julia"
  numpessoas = add_usuario(50, 37, "57 BB 3F 0C", "Julia"); 

  // cria o seg usuario, na pos[1] do vetor, preenchendo nos campos: x = 40, y = 27, cartao  E2 9D 9D 2E, nome "Francisco"
  numpessoas = add_usuario(40, 27, "E2 9D 9D 2E", "Francisco");

  //limpa_eeprom();
  Serial.println("---------------------------------------------------");
  imprime_eeprom();
  escreve_eeprom();

}


void loop() 
{
  lcd.clear();
  digitalWrite (LED_G, LOW);
  digitalWrite (LED_R, HIGH);
 
  // Busca novos cartões 
  // inserida dentro de um laço de repetição, faz com que o modulo fique buscando por um novo cartão a ser lido
  if ( ! mfrc522.PICC_IsNewCardPresent())
  {
    return;
  }
  
  // Seleciona um cartão a ser lido
  // ler a TAG ou cartão que foi encontrado anteriormente.
  if ( ! mfrc522.PICC_ReadCardSerial())
  {
    return;
  }
  
  // checa se o cartao esta cadastrado
  int indice = valida_cartao();
  int posEeprom = indice*TAMUS;

  // encontra o usuario na memoria - retorna a posicao da eeprom referente ao id do usuario
  
  Serial.print("posicao no array: ");
  Serial.print(indice);
  Serial.println();
  
  Serial.println();
  Serial.print("estado antes: ");
  Serial.println(EEPROM[posEeprom]);
  Serial.println();

  // cartao foi aceito - a pessoa esta cadastrada - muda o estado gravado na eeprom
  if (indice >= 0)
  {
    imprime_lcd(indice, users[indice].nome);
    acende_led();
    muda_estado(indice);
  }

  
  
  Serial.println();
  Serial.print("estado depois: ");
  Serial.println(EEPROM[posEeprom]);
  Serial.println();
  imprime_eeprom();
  Serial.println("---------------------------------------------------");

  //Serial.println();
  //imprime_eeprom();

  delay(2000);

}

// cria usuario, na pos[id] do vetor, no campo x = posx, y = posy, associado ao num do cartao
int add_usuario(byte posx, byte posy, String tag, char* nome)
{
    int id = numpessoas;
    numpessoas++;

    users[id].id = id;
    users[id].estado = EEPROM.read(id*TAMUS +1);
    users[id].mesa.x = posx;
    users[id].mesa.y = posy;
    users[id].tag = tag;
    strcpy(users[id].nome, nome);
    return numpessoas;
}


void acende_led()
{
  digitalWrite (LED_G, HIGH);
  digitalWrite (LED_R, LOW);
  delay(2000);
  digitalWrite (LED_G, LOW);
  digitalWrite (LED_R, HIGH);
}


// verifica se o cartao esta cadastrado na eeprom
int valida_cartao()
{
  String conteudo = "";
  byte letra;
  

  // conversao do valor do cartao
  for (byte i = 0; i < mfrc522.uid.size; i++)
  {
    conteudo.concat(String(mfrc522.uid.uidByte[i] < 0x10 ? " 0" : " "));
    conteudo.concat(String(mfrc522.uid.uidByte[i], HEX));
  }
  
  conteudo.toUpperCase();

  for (int i = 0; i < TAM; i++)
  {
    if (users[i].tag ==  conteudo.substring(1))
    {
        //Serial.println("achei usuario na posicao ");
        //Serial.print(i);
        return i;
        
    }
  }
    
   return -1;

}

// muda o estado do usuario - recebe a posicao da eeprom e tamanho do 
void muda_estado(int posicao)
{
  int posEeprom = posicao*TAMUS +1;

  //Serial.print("posEeprom: ");
  //Serial.println(posEeprom);

  //Serial.print("EEPROM[posEeprom]: ");
  //Serial.println(EEPROM[posEeprom]); 
  
  if (EEPROM[posEeprom] == 0)
  {
    EEPROM.update(posEeprom, 1); // recebe endereco e valor a ser gravado
    users[posicao].estado = 1;
  }
  else
  {
    EEPROM.update(posEeprom, 0); // recebe endereco e valor a ser gravado
    users[posicao].estado = 0;
  }
}

// funcao para escrever zero em todas as posicoes da eeprom
void limpa_eeprom()
{
  for (int i = 0; i < EEPROM.length(); i++)
  {
    EEPROM.update(i, 0);
  }
}

// imprime o conteudo de todas as posicoes da eeprom
void imprime_eeprom()
{
  Serial.println("EEPROM: ");
  //for (int i = 0; i < EEPROM.length(); i++)
  for (int i = 0; i < 41; i++)
  {
    Serial.print(EEPROM[i]);
    Serial.print(" |");
  }
  Serial.println(' ');
  Serial.println(' ');
}

// recebe a posicao da eeprom e o nome do usuario - imprime no lcd as mensagens de bem-vindo e tchau
void imprime_lcd(int indice, char* nome)
{
    Serial.println();
    if (EEPROM[indice*TAMUS + 1] == 0)
    {
        Serial.print("Bem-vindo!");
        lcd.begin(16,2);
        lcd.clear();
        lcd.print("Bem-vindo");
        lcd.setCursor(0,1);
        lcd.print(nome);
        lcd.print("!");
    }

    else
    {
      Serial.print("Tchau!");
      lcd.begin(16,2);
      lcd.clear();
      lcd.print("Tchau");
      lcd.setCursor(0,1);
      lcd.print(nome);
      lcd.print("!");
    }
    
    Serial.println();
    Serial.println();
 
}



void escreve_eeprom()
{
  int posEeprom = 0;
  
  for (int i = 0; i < numpessoas; i++)
  {
    EEPROM.update(posEeprom, users[i].id); // recebe o endereco e o valor a ser gravado - igual a EEPROM[posEeprom] = users[i].id;
    posEeprom++;

    EEPROM.update(posEeprom, users[i].estado); // igual a EEPROM[posEeprom] = users[i].estado;
    posEeprom++;
    
    EEPROM.update(posEeprom, users[i].mesa.x); // igual a EEPROM[posEeprom] = users[i].mesa.x;
    posEeprom++;
    
    EEPROM.update(posEeprom, users[i].mesa.y);// igual a EEPROM[posEeprom] = users[i].mesa.y;
    posEeprom++;
    
    for(int j = 0; j < TAMSTR; j++) 
    {
      EEPROM.update(posEeprom, users[i].nome[j]); // igual a EEPROM[posEeprom] = users[i].nome[j];
      posEeprom++;
    }
     
  }

}

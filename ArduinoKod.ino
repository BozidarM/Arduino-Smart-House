//Dodavanje TimerOne biblioteke u projekat
#include <TimerOne.h>

//Definisanje Id-a Arduina
#define ARDUINO_ID 0

//Definisanje konstanti za pinove
#define RELAY_PIN 13
#define DC_MOTOR 9
#define T_ULTRASONIC_SENSOR  11
#define E_ULTRASONIC_SENSOR 10
#define SERVO_MOTOR 5

#define POTENT_PIN A0
#define BRIGHTNESS_PIN A1
#define TEMP_PIN A2

//Definisanje konstanti za stanja za vremenski prekid
#define TRANSMIT 0
#define RECEIVE 1
#define DOOR_STATE 2
#define WAIT 3

//Definisanje i inicijalizovanje potrebnih promenljivih
int relay_state = LOW;
int potent_value = 0;

int old_potent_value = 0;

unsigned long duration;
float distance;
int distance_int = 0;
int door_serial = 0;
int is_opened = 0;
const unsigned long time_interrupt = 2000;
int time_counter = 0;
int current_state;

int relay_state_counter = 0;
int open_door_counter = 0;

void setup()
{
  //Pokretanje serijske komunikacije
  Serial.begin(9600);

  //Postavljanje modova za ulaz i izlaz na potrebnim pinovima
  pinMode(RELAY_PIN, OUTPUT);
  pinMode(DC_MOTOR, OUTPUT);
  pinMode(T_ULTRASONIC_SENSOR, OUTPUT);
  pinMode(E_ULTRASONIC_SENSOR, INPUT);
  pinMode(SERVO_MOTOR, OUTPUT);

  //Setovanje diode na pocetno stanje
  digitalWrite(RELAY_PIN, relay_state);

  //Inicijalizovanje tajmera na 0.002 sekunde
  Timer1.initialize(time_interrupt);
  //Tajmeru se povezuje prekid koji ce da pozove funkciju za vremenski prekid
  Timer1.attachInterrupt(timer_function);

  current_state = TRANSMIT;
}

void loop()
{
  //Uslov koji proverava da li postoji nesto na serijskoj liniji
  if (Serial.available() > 0) {
    //U promenljivu command smestamo string sa serijske linije koji se zavrsava simbolom ;
    String command = Serial.readStringUntil(';');
    //U promenljivu i smestamo indeks prve dvotacke u stringu
    int i = command.indexOf(':');

    if (i > 0) {
      //U promenljivu arduinoId smestamo id arduina koji se nalazi na pocetku stringa
      String arduinoId = command.substring(0, i);
      //String id prebacujemo u integer
      int id = arduinoId.toInt();

      //U promenljivu write_read smestamo treci karakter stringa koji sluzi da kaze
      //da li se radi o citanju ili pisanju
      char write_read = command.charAt(i + 1);

      if (write_read == 'W') {
        // Zahtev za upisivanje

        int s = i + 3;
        //Pronadji indeks dvotacke nakon indeksa 4
        i = command.indexOf(':', s);
        //Dohvati sve do te dvotacke
        String pin = command.substring(s, i);
        //Dohvati string na indeksu 6
        String value = command.substring(i + 1);

        if (id != ARDUINO_ID) {
          // posalji dalje
        } else {
          //Ako je kroz string poslat RELAY_PIN putem serijske komunikacije
          //zameniti stanje diode na tom pinu
          if (pin == "RELAY_PIN") {
            //Menjanje stanja promenlive za stanje diode
            relay_state = !relay_state;
            //Inkrementiranje promenljive koja sluzi za podatak koliko je puta relej promenio stanje
            relay_state_counter++;
            //Upis novog stanja na pin diode
            digitalWrite(RELAY_PIN, relay_state);
            //Ako je kroz string poslat DC_MOTOR pin putem serijske komunikacije
            //promeniti brzinu motora(ventilatora)tom pinu
          } else if (pin == "DC_MOTOR") {
            //Prebacivanje vrednosti motora iz string u integer tip
            int value_int = value.toInt();
            //Mapiranje vrednosti u odgovarajuci opseg
            int m_speed = map(value_int, 0, 100, 0, 255);
            //Upis nove brzine motora
            analogWrite(DC_MOTOR, m_speed);
            //Ako je kroz string poslat SERVO_MOTOR pin putem serijske komunikacije
            //postaviti promenljivu door_serial na 1
          } else if (pin == "SERVO_MOTOR") {
            door_serial = 1;
          }
        }
      }
    }
  }
  //Pozivanje funkcije za promenu brzine dc motora preko potenciometra
  speed_of_dc_motor();
}

//Definisanje funkcije za promenu brzine dc motora preko poteneciometra
void speed_of_dc_motor()
{
  //Smestanje vrednost potenciometra u promenljivu
  potent_value = analogRead(POTENT_PIN);

  //Definisanje delta promenljive koja sluzi za toleranciju potenciometra
  int delta = 10;
  //Mapiranje vrednosti u odgovarajuce opsege
  int motor_speed = map(potent_value, 0, 1023, 0, 255);

  //Uslov koji resava preklapanja promene brzine motora preko potencimetra i
  //preko serijske komunikacije
  if (!(motor_speed >= (old_potent_value - delta) && motor_speed <= (old_potent_value + delta))) {
    analogWrite(DC_MOTOR, motor_speed);
    old_potent_value = motor_speed;
  }
}

//Funkcija koja sluzi za vremenski prekid
void timer_function() {
  time_counter++;
  switch (current_state) {
    //Stanje u kome senzor za merenje razdaljine transmituje signal
    case TRANSMIT:
      //Generisanje triger signala
      digitalWrite(T_ULTRASONIC_SENSOR, HIGH);
      //Posle 8 milisekunid promeniti stanje u RECEIVE
      if (time_counter % 4 == 0) {
        current_state = RECEIVE;
      }
      break;
    //Stanje u kome senzor za merenje razdaljine prima signal koji transmitovao
    case RECEIVE:
      //Triger signal se gasi
      digitalWrite(T_ULTRASONIC_SENSOR, LOW);
      //Osluskuje se echo port i u promenljivu se upisuje vreme koilko je signal putovao
      duration = pulseIn(E_ULTRASONIC_SENSOR, HIGH);
      //Racuna se distanca objekta ili osobe od senzora
      distance = duration * 0.034 / 2;
      current_state = DOOR_STATE;
      break;
    //Stanje koje sluzi za promenu stanja servo motora tj otvaranje i zatvaranje vrata
    case DOOR_STATE:
      distance_int = int(distance);
      //Ako osoba pridje senzoru blize od 5 cm ili sa serijske linije dobijemo naredbu
      //za promenu stanja vrata i ako su vrata zatvorena, otvoriti vrata
      if (((distance_int <= 15) || (door_serial == 1)) && (is_opened == 0)) {
        //Inkrementiranje promenljive koja sluzi za informaciju koli puta su vrata otvorena
        open_door_counter++;
        analogWrite(SERVO_MOTOR, 250);
        //Setuje se promenljiva koja pokazuje da li su vrata otvorena na 1
        is_opened = 1;
        //Promelnjiva koja sluzi da nam kaze da li imamo naredbu sa serijskog porta se vraca u pocetno stanje
        door_serial = 0;
      }
      //Ako se osoba udalji od senzora vise od 5 cm ili sa serijske linije dobijemo naredbu
      //za promenu stanja vrata i ako su vrata otvorena, zatvoriti vrata
      else if (((distance_int > 15) || (door_serial == 1)) && (is_opened == 1)) {;
        analogWrite(SERVO_MOTOR, 0);
        //Setuje se promenljiva koja pokazuje da li su vrata otvorena na 0
        is_opened = 0;
        door_serial = 0;
      }

      current_state = WAIT;
      break;
    //Stanjeu kome timer ceka da pono transmituje signal
    case WAIT:
      //Nakon 1 sekunde ponovo prebaci u stanje za TRANSMIT
      if (time_counter % 500 == 0) {
        current_state = TRANSMIT;
      }
      break;
  }
  //Uslov koji se izvrsava svakog minuta i salje podatke web serveru
  if (time_counter % 15000 == 0) {
    //Dohvatanje vrednosti osvetljenja uz pomoc funkcije
    int bri_value = get_brightness_in_precentage();
    //Slanje vrednosti osvetljena web serveru
    send_data("BRIGHTNESS_PIN", (float)bri_value);

    //Dohvatanje vrednosti temperature uz pomoc funkcije
    float temp_value = get_temperature_in_c();
    //Slanje vrednosti temperature web serveru
    send_data("TEMP_PIN", temp_value);

    //Slanje ostalih vrednosti
    send_data("DOOR_COUNTER", open_door_counter);
    send_data("RELAY_COUNTER", relay_state_counter);
    send_data("LIGHT", relay_state);
    send_data("SERVO_MOTOR", is_opened);
  }

}

//Funkcija za dohvatanje temperature
float get_temperature_in_c() {
  //Citamo vrednost sa senzora za temperaturu
  int analog_temp_value = analogRead(TEMP_PIN);

  //Formula za dobijanje temperature u C
  float real_temp = 500 * analog_temp_value / 1023.0f;

  return real_temp;
}

int get_brightness_in_precentage() {

  //Citanje vrednosti za fotootpornika
  int analog_bri_value = analogRead(BRIGHTNESS_PIN);
  //Mapiranje opsega
  int brightness_in_precentage = map(analog_bri_value, 1, 310, 0, 100);

  return analog_bri_value;

}

//Funkcija za kreiranje formata za slanje podataka serveru
void send_data(String pin, float value) {
  String s = String(ARDUINO_ID) + ":" + String(pin) + "|" + String(value) + ";";
  Serial.print(s);
}

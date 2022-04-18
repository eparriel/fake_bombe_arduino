#include <Keypad.h>


#define BUTTON_PIN 10

#define GREEN_LED 4
#define RED_LED 3
#define PIN_BUZZER 2

char key;

//pave numerique
const byte ROWS = 3;
const byte COLS = 3;

char keysmap[ROWS][COLS] = {
        {'1', '2', '3'},
        {'4', '5', '6'},
        {'7', '8', '9'}
};

byte rowPins[ROWS] = {10,9,8};
byte colPins[COLS] = {7,6,5};

Keypad keypad = Keypad( makeKeymap(keysmap), rowPins, colPins, ROWS, COLS);

#define DS_pin 13
#define STCP_pin 12
#define SHCP_pin 11 

const int dec_digits [10] {1,79,18,6,76,36,32,15,0,4};

class Timer {
    unsigned long startTime = 0;
public:
    void start();
    unsigned long elapsed();
};
void Timer::start() {
    startTime = millis();
}
unsigned long Timer::elapsed() {
    return millis() - startTime;
}

enum State {
    UNKNOWN,
    SUCCESS,
    BOMB_ON,
    BOOM,
    CREATE_CODE,
    CHECK_INPUT,
    WARNING
};

String statesName[] = {"UNKNOWN", "SUCCESS", "BOMB_ON", "BOOM", "CREATE_CODE", "CHECK_INPUT", "WARNING"};

class Fsm {
    State _current_state = UNKNOWN;
public:
    void checkState(State source_state, State target_state, bool condition);
    State getCurrentState();
    static String stateToString(State state);
};

void Fsm::checkState(State source_state, State target_state, bool condition) {
    if (source_state == _current_state && condition) {
        _current_state = target_state;
        Serial.print("Source state :");
        Serial.println(stateToString(source_state));
        Serial.print("Target state :");
        Serial.println(stateToString(target_state));
        Serial.println("--------");
    }
}

State Fsm::getCurrentState() {
    return _current_state;
}

String Fsm::stateToString(State state) {
    return statesName[state];
}

int arduinoAtoi(String str) {
    int res = 0;

    for (int i = 0; str[i] != '\0'; ++i) {
        res = res * 10 + str[i] - '0';
    }
    return res;
}

Timer t1;

class Bombe {
    String code = "";
    String attempt = "";
    bool explosed = false;
    unsigned long givenTime = 50000;
    int nbMaxTentative = 4;
    int inputLength = 0;
    int nbTentative = 0;

public:
    void createCode();
    void checkInput();
    void bombOn();
    void warning();
    void boom();
    void deactivated();
    void displayTime();
    void giveTips();

    bool getExplosed();
    unsigned long getTime();
    String getCode();
    int getNbMaxTentative();
    int getInputLength();
    int getNbTentative();
};

void Bombe::createCode() {
    randomSeed(analogRead(0));
    for (int i = 0; i < 4; i++) {
        Bombe::code.concat(random(1, 9));
    }
    Serial.print("Le code se situe entre ");
    Serial.print(arduinoAtoi(code)-random(3, 15));
    Serial.print(" et ");
    Serial.println(arduinoAtoi(code)+random(3, 15));
    Serial.print("Vous avez: ");
    Serial.print(givenTime/1000);
    Serial.println("secondes pour le trouver.");
//    Serial.print("code: ");
//    Serial.println(code);
}

void Bombe::checkInput() {
    if (attempt == code) {
        explosed = true;
    } else {
        giveTips();
        nbTentative++;
        givenTime -= 10000;
        tone(PIN_BUZZER, 200, 50);
        attempt = "";
        inputLength = 0;
    }
}

void Bombe::bombOn() {
    key = keypad.getKey();
    if (key) {
        attempt.concat(key);
        Serial.print("Vous avez appuyez sur: ");
        Serial.println(key);
        inputLength++;
    }
    displayTime();
}

void Bombe::boom() {
    digitalWrite(RED_LED,HIGH);
    tone(PIN_BUZZER, 800, 50);
    delay(5000);
    exit(0);
}

void Bombe::deactivated() {
    digitalWrite(GREEN_LED,HIGH);
    delay(4000);
    exit(0);
}

bool Bombe::getExplosed() {
    return explosed;
}

unsigned long Bombe::getTime() {
    return givenTime;
}

String Bombe::getCode() {
    return code;
}

int Bombe::getNbMaxTentative() {
    return nbMaxTentative;
}

void Bombe::displayTime() {
    if (t1.elapsed() % 1000 == 0) {
        unsigned long time = givenTime/1000 - t1.elapsed()/1000;
        unsigned long sec = time%10;
        unsigned long dizaine = time/10;
        digitalWrite(STCP_pin,LOW);
        shiftOut(DS_pin, SHCP_pin, LSBFIRST,dec_digits[sec]);
        shiftOut(DS_pin, SHCP_pin, LSBFIRST,dec_digits[dizaine]);
        digitalWrite(STCP_pin, HIGH);
    }
}

void Bombe::giveTips() {
    if (arduinoAtoi(attempt) > arduinoAtoi(code))  {
        Serial.println("Le nombre est inferieur");
    } else {
        Serial.println("Le nombre est superieur");
    }
}

int Bombe::getInputLength() {
    return inputLength;
}

int Bombe::getNbTentative() {
    return nbTentative;
}

void Bombe::warning() {
    if (t1.elapsed() % 1000 == 0) {
        tone(PIN_BUZZER, 100, 100);
    }
    bombOn();
}

Fsm fsm;
Bombe bombe;

void getInputs() {
}

void setState(int state) {
    // execute matching state
    //Tout eteindre avant de changer d'etat
    pinMode(BUTTON_PIN, INPUT_PULLUP);

    switch (state) {
        case UNKNOWN:
            break;
        case CREATE_CODE:
            t1.start();
            bombe.createCode();
            break;
        case CHECK_INPUT:
            bombe.checkInput();
            break;
        case BOMB_ON:
            bombe.bombOn();
            break;
        case WARNING:
            bombe.warning();
            break;
        case BOOM:
            bombe.boom();
            break;
        case SUCCESS:
            bombe.deactivated();
            break;
    }
}

void setup()
{
    pinMode(LED_BUILTIN, OUTPUT);
    char BUZZER = 3;
    Serial.begin(9600);
    pinMode(DS_pin, OUTPUT);
    pinMode(STCP_pin, OUTPUT);
    pinMode(SHCP_pin, OUTPUT);
    randomSeed(analogRead(0));
}

void loop()
{
    // get all inputs
    getInputs();
// run fsm

    fsm.checkState(UNKNOWN, CREATE_CODE, true);
    fsm.checkState(CREATE_CODE, BOMB_ON, bombe.getCode() != "");
    fsm.checkState(CHECK_INPUT, WARNING, !bombe.getExplosed() && bombe.getNbTentative() < bombe.getNbMaxTentative() && t1.elapsed() + 10000 >= bombe.getTime());
    fsm.checkState(CHECK_INPUT, BOMB_ON, !bombe.getExplosed() && bombe.getNbTentative() < bombe.getNbMaxTentative());
    fsm.checkState(BOMB_ON, BOOM, t1.elapsed() >= bombe.getTime());
    fsm.checkState(WARNING, BOOM, t1.elapsed() >= bombe.getTime());
    fsm.checkState(BOMB_ON, CHECK_INPUT, bombe.getInputLength() == 4);
    fsm.checkState(WARNING, CHECK_INPUT, bombe.getInputLength() == 4);
    fsm.checkState(BOMB_ON, WARNING, t1.elapsed() + 10000 >= bombe.getTime());
    fsm.checkState(CHECK_INPUT, BOOM, bombe.getNbTentative() >= bombe.getNbMaxTentative() || t1.elapsed() >= bombe.getTime());
    fsm.checkState(CHECK_INPUT, SUCCESS, bombe.getExplosed());
    setState(fsm.getCurrentState());
}

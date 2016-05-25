#include "SPI.h"

/*Variablen Deklaration*/
/*Timer & LED*/
volatile byte color = 0;
volatile byte com = 1;
byte my_green[8];
byte my_red[8];
byte my_blue[8];
byte op_green[8];
byte op_red[8];
byte op_blue[8];
byte ctrred = 0;

const boolean DEMO = 0;
/*Game*/
const bool oneMatrixMode = true;
const bool debug = true;
bool sound = false;
bool cursorOnHitMatrix = false;
bool currentID;
bool currentPlayer;



const int xPinCtl = A0;
const int yPinCtl = A1;
const int zPinCtl = 8;

const int dPinBuzzer = 7;

const int ticksPerSecond = 5;
const int sensiCtl = 4;

// 4 World Dimensions - World1,World2,Hit1,Hit2
byte myWorld[4][8][8];

/* Controller Helper */
enum ControlEvents
{
	NONE,
	LEFT,
	RIGHT,
	DOWN,
	UP,
	CLICK
};

ControlEvents myControlEvent = NONE;

int xPosCursor = 0;
int yPosCursor = 0;
bool cursorLocked = false;

/* ControllerEventCallbacks Left,Right,Down,Up,Click,None */
void ctlL() {
	myControlEvent = LEFT;
	if (!cursorLocked) {
		xPosCursor--;
		if (xPosCursor == -1)xPosCursor = 7;
	}
}
void ctlR() {
	myControlEvent = RIGHT;
	if (!cursorLocked) {
		xPosCursor++;
		if (xPosCursor == 8)xPosCursor = 0;
	}
}
void ctlD() {
	myControlEvent = DOWN;
	if (!cursorLocked) {
		yPosCursor--;
		if (yPosCursor == -1)yPosCursor = 7;
	}
}
void ctlU() {
	myControlEvent = UP;
	if (!cursorLocked) {
		yPosCursor++;
		if (yPosCursor == 8)yPosCursor = 0;
	}
}
void ctlC() {
	myControlEvent = CLICK;
}

/* XY Controller */
void controlXY(int x, int y) {
	//activationRange: Joystick-Push-Range for Activation of Control
	int activationRange = 1500 / (sensiCtl >= 3 ? sensiCtl : 3);

	//XControl
	if (x < 512 - activationRange)
		ctlL();
	else if (x > 512 + activationRange)
		ctlR();

	//YControl
	if (y < 512 - activationRange)
		ctlU();
	else if (y > 512 + activationRange)
		ctlD();
}

void ledCtl() {
	if (!currentPlayer || currentID==-1)return;
	// Shift wegen LED-Reihen vertauscht
	setLedRow(0,0);
	setLedRow(1,1);
	setLedRow(3,2);
	setLedRow(2,3);
	setLedRow(4,4);
	setLedRow(5,5);
	setLedRow(7,6);
	setLedRow(6,7);
}

void setWorldLedMatrix(byte &red, byte &green, byte &blue, int j, int s) {
	if (!cursorOnHitMatrix && j == xPosCursor && s == yPosCursor) {
		red -= (1 << j);
		blue -= (1 << j);
	}
	else {
		switch (myWorld[currentID][j][s])
		{
		case 2: // kaputtes Schiff
			red -= (1 << j);
			break;
		case 1: // Schiff
			green -= (1 << j);
			break;
		case 0: // Wasser
		default:
			blue -= (1 << j);
			break;
		}
	}
}

void setHitLedMatrix(byte &red, byte &green, byte &blue, int j, int s) {
	if (cursorOnHitMatrix && j == xPosCursor && s == yPosCursor) {
		red -= (1 << j);
		blue -= (1 << j);
	}
	else {
		switch (myWorld[currentID + 2][j][s])
		{
		case 2: // Treffer
			red -= (1 << j);
			break;
		case 1: // Pumpe
			green -= (1 << j);
			break;
		case 0: // Wasser
		default:
			blue -= (1 << j);
			break;
		}
	}
}

void setLedRow(int i,int s) {
	my_red[i] = (byte)255;
	my_green[i] = (byte)255;
	my_blue[i] = (byte)255;
	op_red[i] = (byte)255;
	op_green[i] = (byte)255;
	op_blue[i] = (byte)255;

	for (int j = 0; j < 8; j++) {
		setWorldLedMatrix(op_red[i], op_green[i], op_blue[i], j, s);
		setHitLedMatrix(my_red[i], my_green[i], my_blue[i], j, s);
	}
}

void setMenuInWorld() {}

/* Input Controller */
int inputCtl(bool waitForFinalClick = false, int id = -1) {
	myControlEvent = NONE;
	while (myControlEvent == NONE) {
		//Click abfangen
		if (digitalRead(zPinCtl) == 0)
			ctlC();
		//XY Steuerung
		controlXY(analogRead(xPinCtl), analogRead(yPinCtl));
		if (debug && myControlEvent != CLICK && myControlEvent != NONE && id != -1)
			printWorld(myWorld[id]);
		//Auf Finalen Click warten
		if (myControlEvent != CLICK && waitForFinalClick)
			myControlEvent = NONE;
		//Taktung steuern
		delay(1000 / ticksPerSecond);
	}
	return myControlEvent;
}

/* Sound Creator */
void playMissileSound(bool hit = false) {
  // Pfeifen
  for (int i = 3750; i > 2250; i -= 25)
  {
    tone(dPinBuzzer, i + random(-24, 0));
    delay((i / random(160, 320)));
  }
  if (hit) {
    for (int j = 0; j < 10; j++)
    {
      tone(dPinBuzzer, random(50, 150));
      delay(random(30, 60));
    }
  }
  noTone(dPinBuzzer);
}

/* playGame */
void fireMissile(int id, boolean player)
{
	setIDandPlayer(id, player);
	int myEnemyID;
	// Wähle Feld auf Hitarray
	if (id == 0)
		myEnemyID = 1;
	else if (id == 1)
		myEnemyID = 0;

	int myX;
	int myY;

	if (!player) {
		//TODO AI
		do {
			myX = random(0, 7);
			myY = random(0, 7);
		} while (myWorld[id + 2][myX][myY] != 0);
	}
	else {
		inputCtl(true, id + 2);
		myX = xPosCursor;
		myY = yPosCursor;
	}

	if (myWorld[myEnemyID][myX][myY] == 1) {
		myWorld[myEnemyID][myX][myY] = 2;
		myWorld[id + 2][myX][myY] = 2;
		if (sound)playMissileSound(true);
	}
	else {
		myWorld[id + 2][myX][myY] = 1;
		if (sound)playMissileSound();
	}
}

/* Game Controller */
void gameCtl(bool solo = true) {
	bool player1 = true;
	bool player2;
	if (solo) {
		if(debug)Serial.println("I'm in Solo Mode");
		player2 = false;
	}
	else {
		if (debug)Serial.println("I'm in Versus Mode");
		player2 = true;
	}

	initWorld(0, player1);
	initWorld(1, player2);

	cursorOnHitMatrix = true;
	while (worldHasShips(0) && worldHasShips(1))
	{
    fireMissile(0,player1);
	delay(750);
    fireMissile(1,player2);
	}
	cursorOnHitMatrix = false;
}

void setIDandPlayer(int id, bool player) {
	currentID = id;
	currentPlayer = player;
}

/* World Initializer */
void initWorld(int id, bool player) {
	setIDandPlayer(id, player);
	for (int i = 0; i < 8; i++)
	{
		for (int j = 0; j < 8; j++)
		{
			myWorld[id][i][j] = 0;
			myWorld[2 + id][i][j] = 0;
		}
	}
	placeShips(id, player);
}

/* World ShipCheck */
bool worldHasShips(int id) {
	for (int i = 0; i < 8; i++)
	{
		for (int j = 0; j < 8; j++)
		{
			if (myWorld[id][i][j] == 1)
				return true;
		}
	}
	return false;
}

/* Place 6 ships per Player*/
void placeShips(int id, bool player) {
	if (player) {
		placePlayerShip(id, 4);
		placePlayerShip(id, 4);
		placePlayerShip(id, 3);
		placePlayerShip(id, 3);
		placePlayerShip(id, 2);
		placePlayerShip(id, 2);
	}
	else {
		//TODO AI
		placeAIShip(id, 4);
		placeAIShip(id, 4);
		placeAIShip(id, 3);
		placeAIShip(id, 3);
		placeAIShip(id, 2);
		placeAIShip(id, 2);
	}
}

/* Inputwrapper for placing a player ship with placeShip*/
void placePlayerShip(int id, int size) {
	if (debug)Serial.println("Richtung:");
	cursorLocked = true;
	int dir = inputCtl();
	cursorLocked = false;
	bool placed = false;
	while (!placed) {
		if (debug)Serial.println("Position:");
		inputCtl(true, id);
		placed = placeShip(id, size, xPosCursor, yPosCursor, dir);
	}
}

void placeAIShip(int id, int size) {
	int myX;
	int myY;
	int myDir;
	do {
		myX = random(0, 7);
		myY = random(0, 7);
		myDir = random(LEFT, UP);
	} while (!placeShip(id, size, myX, myY, myDir));
}

/* Place a single ship, check for other ships and borders*/
bool placeShip(int id, int size, int xPos, int yPos, int dir) {
	int x = 0;
	int y = 0;
	bool canPlace = true;
	switch (dir) {
	case LEFT:
		x = -1;
		canPlace = (xPos - (size-1)) > -1;
		break;
	case RIGHT:
		x = 1;
		canPlace = (xPos + (size - 1)) < 8;
		break;
	case DOWN:
		y = -1;
		canPlace = (yPos - (size - 1)) > -1;
		break;
	case UP:
	default:
		y = 1;
		canPlace = (yPos + (size - 1)) < 8;
		break;
	}

	/*
		# = Ship
		O = CheckArea
		
		Example Ship Size = 3

		OOOOO
		O###O
		OOOOO
	*/
	for (int s = -1; s <= size; s++)
	{
		for (int drift = -1; drift <= 1; drift++)
		{
			if (myWorld[id][limit(xPos + (y*drift) + (x*s))][limit(yPos + (x*drift) + (y*s))] != 0)
				canPlace = false;
		}
	}

	if (canPlace) {
		for (int s = 0; s < size; s++)
		{
			myWorld[id][xPos + (x*s)][yPos + (y*s)] = 1;
		}
		return true;
	}
	else {
		return false;
	}
}

int limit(int val) {
	if (val > 7)
		return 7;
	if (val < 0)
		return 0;
}

//LED Ersatzfunktionen
void printLabeled(String name, int val) {
	Serial.print(name + ": ");
	Serial.println(val);
}

void printWorld(byte world[8][8]) {
	if (debug)Serial.println("######8!");
	for (int i = 7; i >= 0; i--)
	{
		if (debug) {
			for (int j = 0; j < 8; j++)
			{
				if (j == xPosCursor && i == yPosCursor) {
					if (debug)Serial.print("x");
				}
				else {
					if (debug)Serial.print(world[j][i]);
				}
			}
			if (debug)Serial.println();
		}
		
			printLabeled("R", my_red[i]);
			printLabeled("G", my_green[i]);
			printLabeled("B", my_blue[i]);
			printLabeled("xPos", xPosCursor);
			printLabeled("yPos", yPosCursor);
		
	}
	if (debug)Serial.println("!0######");
}

/* Setup */
void setup()
{
	/*----------------------Interrupt Initialisierung--------------------*/
	// pinMode(13, OUTPUT);
	cli();//Interrupts verbieten (CLear Interrupts)

		  //Timer 1 inititalisieren
	TCCR1A = 0; //Quelle: Systemtakt (=16MHz)
	TCCR1B = 0; //Zählwert: 0
	TCNT1 = 0; //Modus: Default

			   //Vergleichsregister initialisieren (kleinerer Wert = häufigere Interrupts)
			   //Wert wird/wurde experimentell ermittelt
	OCR1A = 10;

	//Bit-gehacke um die Flags im TCCR1B-Register zu setzen
	//TCCR1B = TCCR1B | (1 << $name_einer_Konstante) 
	//1 wird an die entsprechende Stelle geschoben, dann mit dem Registerinhalt bitweise verodert und ins Register geschrieben
	TCCR1B |= (1 << WGM12); //CTC-Modus aktivieren
	TCCR1B |= (1 << CS12) | (1 << CS10);  //prescaler =1024
	TIMSK1 |= (1 << OCIE1A); //Timer Compare Interrupt einschalten 

	sei();//Interrupts erlauben (SEt Interrupts)
		  /*----------------------Ende Interrupt Initialisierung----------------*/

		  /*----------------------SPI Initialisierung---------------------------*/
	SPI.begin();
	SPI.setDataMode(SPI_MODE0);
	SPI.setBitOrder(MSBFIRST);

	/*----------------------Ende SPI Initialisierung----------------------*/
	com = 1;
	color = 0;
	/*Display-Demo-Daten*/
	//Anzeige testen....
	if (DEMO) {
		for (int i = 0; i<8; i++) {
			my_green[i] = 0b10110111;
			my_red[i] = 0b11111111;
			my_green[i] = 0b11100100;
			op_green[i] = 0x8E;
			op_red[i] = 0x70;
			op_blue[i] = 0x22;
		}
	}
	/*ende Display-Demo-Daten*/

	//Not pressed Joystick = 1 on InputPin zPinCtl
	pinMode(zPinCtl, INPUT);
	digitalWrite(zPinCtl, HIGH);

	//Buzzer
	pinMode(dPinBuzzer, OUTPUT);

	if (debug) Serial.begin(9600);
}

/* Loop */
void loop()
{
	//Launcher
	Serial.println("Solo oder Versus?");
	inputCtl(false);
	if (myControlEvent == LEFT) {
		//Play Solo
		gameCtl();
	}
	else if (myControlEvent == RIGHT) {
		//Play Versus
		gameCtl(false);
	}
	else if (myControlEvent == UP) {
		sound = !sound;
	}
	else if (myControlEvent == DOWN) {
		playMissileSound(true);
	}
}

/*----------------------Interrupt Routine----------------------------*/
/*  Zeilenweises Ansteuern der RGB-LED-Matritzen via Schieberegister
*   (8* 74hc595) Reihenfolge:
*   Arduino -> op_blue -> op_red -> op_green -> com_Anode -> my_blue ->
*   my_red -> my_green -> com_Anode
*
*   Zeilenweises, synchrones durchgehen der Matritzen
*   => Reihenfolge der Ausgabe
*   Zeile i - my_green - my_red - my_blue - Zeile i - op_green - op_red - op_blue
*
*  mosi(11) auf data(weiss)
*  sck(13) auf sh_cp(gruen)
*  ss(10) auf st_cp(gelb/grau)
*  ctrred: counter damit rot nicht jeden Durchlauf angesteuert wird, ist sonst zu hell.
*/
ISR(TIMER1_COMPA_vect) {
	ledCtl();
	digitalWrite(10, LOW); //Schieberegister Ausgang blocken
						   //Daten schieben
	SPI.transfer(0x02);
	SPI.transfer(my_green[color]);
	if (ctrred == 3) {
		SPI.transfer(my_red[color]);
	}
	else {
		SPI.transfer(0xFF);
	}
	SPI.transfer(my_blue[color]);
	SPI.transfer(com);
	SPI.transfer(op_green[color]);
	if (ctrred == 3) {
		SPI.transfer(op_red[color]);
		ctrred = 0;
	}
	else {
		SPI.transfer(0xFF);
	}
	ctrred++;
	SPI.transfer(op_blue[color]);
	//Ende Daten schieben
	digitalWrite(10, HIGH); //Schieberegister Ausgang aktivieren (= neue Anzeige wird geschaltet)

	color++; //naechste Zeile Farben
	if (color >= 8) color = 0; //letzte Zeile Farben -> erste Zeile Farben
	com = 1 << color; //Ansteuerung fuer gemeinsame Zeile
}
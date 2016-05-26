#include "SPI.h"

/*
	Projekt: Battleship

	Marcus Siesenop
	Mattheus van Iterson
*/

/*Variablen Deklaration & Initialisierung*/

/* Debug Options */
const bool oneMatrixMode = true;
const bool debug = false;
const bool displayDemo = false;

/* Interrupt Variables*/
volatile byte color = 0;
volatile byte com = 1;

/* RGB Storage for World and Hit Matrix Displays */
byte my_green[8];
byte my_red[8];
byte my_blue[8];
byte op_green[8];
byte op_red[8];
byte op_blue[8];
byte ctrred = 0;

/* Hardware/Pin Configuration*/
const int xPinCtl = A0;
const int yPinCtl = A1;
const int zPinCtl = 8;
const int dPinBuzzer = 7;
const int dPinRegisterOut = 10;

/* Game Configuration */
const int ticksPerSecond = 8;
const int sensiCtl = 4;
const int shipSizes[6] = {4,4,3,3,2,2};


/* Game Run Variables */
// Cursor Position
int xPosCursor = 0;
int yPosCursor = 0;
// 6 World Dimensions - World1,World2,Hit1,Hit2,Reg1,Reg2
byte myWorld[6][8][8];
// Ships left on Fields
bool shipsLeft[2][6] = { { true,true,true,true,true,true },{ true,true,true,true,true,true } };

bool cursorLocked = false;
bool sound = false;
bool cursorOnHitMatrix = false;
bool isCursorPos;
bool currentID;
bool currentPlayer;

/*-------------------------------------------------------------------*/
/* #START Input Controller & Input Helper */

/* Enum for Directions */
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

/* #END Input Controller & Input Helper */
/*-------------------------------------------------------------------*/



/*-------------------------------------------------------------------*/
/* #START Display & Sound Helper */

/* Control LEDs with shift*/
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

/* set RGB for a single Row with custom row shift*/
void setLedRow(int i,int s) {
	my_red[i] = (byte)255;
	my_green[i] = (byte)255;
	my_blue[i] = (byte)255;
	op_red[i] = (byte)255;
	op_green[i] = (byte)255;
	op_blue[i] = (byte)255;

	// Locked Cursor Helper

	for (int j = 0; j < 8; j++) {
		//World
		isCursorPos = (cursorLocked && ((j == xPosCursor - 1 && s == yPosCursor) || (j == xPosCursor + 1 && s == yPosCursor) || (j == xPosCursor && s == yPosCursor + 1) || (j == xPosCursor && s == yPosCursor - 1))) || (!cursorLocked && (j == xPosCursor && s == yPosCursor));

		if (!cursorOnHitMatrix && isCursorPos) {
			my_red[i] -= (1 << j);
			my_blue[i] -= (1 << j);
		}
		else {
			switch (myWorld[currentID][j][s])
			{
			case 2: // kaputtes Schiff
				my_red[i] -= (1 << j);
				break;
			case 1: // Schiff
				my_green[i] -= (1 << j);
				break;
			case 0: // Wasser
			default:
				my_blue[i] -= (1 << j);
				break;
			}
		}

		//Hit
		if (cursorOnHitMatrix && j == xPosCursor && s == yPosCursor) {
			op_red[i] -= (1 << j);
			op_blue[i] -= (1 << j);
		}
		else {
			switch (myWorld[currentID + 2][j][s])
			{
			case 2: // Treffer
				op_red[i] -= (1 << j);
				break;
			case 1: // Pumpe
				op_green[i] -= (1 << j);
				break;
			case 0: // Wasser
			default:
				op_blue[i] -= (1 << j);
				break;
			}
		}
	}
}

/* Startmenu */
void chooseMenu(int menuPoint) {
	for (int i = 0; i < 8; i++)
	{
		for (int j = 0; j < 8; j++)
		{
			myWorld[0][i][j] = 0;
		}

	}
	if (menuPoint == 0) {
		const byte anzahlMappings = 17;
		byte xMapping[] = { 1,1,1,1,1,2,3,4,4,4,4,4,6,6,6,6 };
		byte yMapping[] = { 2,3,4,5,6,4,4,2,3,4,5,6,2,3,4,6 };
		for (int n = 0; n < anzahlMappings; n++)
		{
			for (int i = 0; i < 8; i++)
			{
				for (int j = 0; j < 8; j++)
				{
					if (i == xMapping[n] && j == yMapping[n])
						if(sound)
							myWorld[0][i][j] = 1;
						else
							myWorld[0][i][j] = 2;
				}
			}
		}
	}
	else if (menuPoint == 1) {
		const byte anzahlMappings = 11;
		byte xMapping[] = { 4,3,2,2,2,3,4,4,4,3,2 };
		byte yMapping[] = { 5,5,5,4,3,3,3,2,1,1,1 };
		for (int n = 0; n < anzahlMappings; n++)
		{
			for (int i = 0; i < 8; i++)
			{
				for (int j = 0; j < 8; j++)
				{
					if (i == xMapping[n] && j == yMapping[n])
						myWorld[0][i][j] = 1;
				}
			}
		}
	}
	else if (menuPoint == 2) {
		const byte anzahlMappings = 12;
		byte xMapping[anzahlMappings] = { 1,1,1,2,2,3,4,5,5,6,6,6 };
		byte yMapping[anzahlMappings] = { 6,5,4,3,2,1,1,2,3,4,5,6 };
		for (int n = 0; n < anzahlMappings; n++)
		{
			for (int i = 0; i < 8; i++)
			{
				for (int j = 0; j < 8; j++)
				{
					if (i == xMapping[n] && j == yMapping[n])
						myWorld[0][i][j] = 1;
				}
			}
		}
	}
	else if (menuPoint == 3) {
		const byte anzahlMappings = 20;
		byte xMapping[anzahlMappings] = { 0,0,1,1,2,2,3,3,3,3,3,3,5,5,6,6,7,7,7,7 };
		byte yMapping[anzahlMappings] = { 3,4,3,4,2,5,6,1,5,2,4,3,3,4,1,6,3,4,2,5 };
		for (int n = 0; n < anzahlMappings; n++)
		{
			for (int i = 0; i < 8; i++)
			{
				for (int j = 0; j < 8; j++)
				{
					if (i == xMapping[n] && j == yMapping[n])
						myWorld[0][i][j] = 1;
				}
			}
		}
	}
	else if (menuPoint == 4) {
		const byte anzahlMappings = 12;
		byte xMapping[anzahlMappings] = { 0,0,1,1,2,2,3,3,3,3,3,3};
		byte yMapping[anzahlMappings] = { 3,4,3,4,2,5,6,1,5,2,4,3};
		for (int n = 0; n < anzahlMappings; n++)
		{
			for (int i = 0; i < 8; i++)
			{
				for (int j = 0; j < 8; j++)
				{
					if (i == xMapping[n] && j == yMapping[n])
						myWorld[0][i][j] = 1;
				}
			}
		}
	}
}

/* Sound Creator */
/* Rocket Sound */
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

/* Ship Destruction Buzz*/
void shipDestroyed() {
	tone(dPinBuzzer, 3000, 100);
	delay(150);
	noTone(dPinBuzzer);
}

/* #END World Initializer Helper */
/*-------------------------------------------------------------------*/



/*-------------------------------------------------------------------*/
/* #START Game Controller & Game Helper */

/* Game Controller */
void gameCtl(bool solo = true) {
	bool player1 = true;
	bool player2 = !solo;

	if (debug)Serial.println(solo ? "1-Spieler-Modus" : "2-Spieler-Modus");

	initWorld(0, player1);
	initWorld(1, player2);

	cursorOnHitMatrix = true;

	bool gameIsRunning = true;
	while (gameIsRunning)
	{
    fireMissile(0,player1);
	gameIsRunning = worldHasShips(1) && worldHasShips(0);
	delay(750);
    fireMissile(1,player2);
	gameIsRunning = worldHasShips(0) && worldHasShips(1);
	}
	cursorOnHitMatrix = false;
	if (debug) {
		if (worldHasShips(0))
			Serial.println("Spieler 1 gewinnt!");
		if (worldHasShips(1))
			Serial.println("Spieler 2 gewinnt!");
	}
}

/* Set the ID and Type of current player */
void setIDandPlayer(int id, bool player) {
	currentID = id;
	currentPlayer = player;
}

/* World ShipCheck */
bool worldHasShips(int id) {
	bool shipsLeftCheck[6] = { false,false,false,false,false,false };
	for (int i = 0; i < 8; i++)
	{
		for (int j = 0; j < 8; j++)
		{
			for (int k = 1; k < 7; k++)
			{
				if(myWorld[id + 4][i][j] == k)
					shipsLeftCheck[k-1] = true;
			}
		}
	}
	for (int k = 0; k < 6; k++)
	{
		if (shipsLeft[id][k] != shipsLeftCheck[k]) {
			shipsLeft[id][k] = shipsLeftCheck[k];
			shipDestroyed();
		}
	}
	bool ret = false;
	for (int k = 0; k < 6; k++)
	{
		ret = ret || shipsLeft[id][k];
	}
	return ret;
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
		myWorld[myEnemyID+4][myX][myY] = 0;
		myWorld[id + 2][myX][myY] = 2;
		if (sound)playMissileSound(true);
	}
	else {
		myWorld[id + 2][myX][myY] = 1;
		if (sound)playMissileSound();
	}
}

/* #END World Initializer Helper */
/*-------------------------------------------------------------------*/



/*-------------------------------------------------------------------*/
/* #START World Initializer Helper */

void initWorld(int id, bool player) {
	setIDandPlayer(id, player);
	for (int i = 0; i < 8; i++)
	{
		for (int j = 0; j < 8; j++)
		{
			myWorld[id][i][j] = 0;
			myWorld[id+2][i][j] = 0;
			myWorld[id+4][i][j] = 0;
		}
	}
	placeShips();
}

/* Place Ship Wrapper Place 6 ships per Player*/
void placeShips() {
	if (currentPlayer) {
		for (int i = 0; i < 6; i++)
		{
			placePlayerShip(i+1, shipSizes[i]);
		}
	}
	else {
		//TODO AI
		for (int i = 0; i < 6; i++)
		{
			placeAIShip(i+1, shipSizes[i]);
		}
	}
}

/* Inputwrapper for placing a player ship with placeShip */
void placePlayerShip(int shipNum, int size) {
	bool placed = false;
	while (!placed) {
		if (debug)Serial.println("Richtung:");
		cursorLocked = true;
		int dir = inputCtl();
		cursorLocked = false;
		if (debug)Serial.println("Position:");
		inputCtl(true, currentID);
		placed = placeShip(shipNum, size, xPosCursor, yPosCursor, dir);
	}
}

/* AIWrapper for placing a AI ship with placeShip*/
void placeAIShip(int shipNum, int size) {
	int myX;
	int myY;
	int myDir;
	do {
		myX = random(0, 7);
		myY = random(0, 7);
		myDir = random(LEFT, UP);
	} while (!placeShip(shipNum, size, myX, myY, myDir));
}

/* Place a single ship, check for other ships and borders */
bool placeShip(int shipNum, int size, int xPos, int yPos, int dir) {
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
			if (myWorld[currentID][limit(xPos + (y*drift) + (x*s))][limit(yPos + (x*drift) + (y*s))] != 0)
				canPlace = false;
		}
	}

	if (canPlace) {
		for (int s = 0; s < size; s++)
		{
			myWorld[currentID][xPos + (x*s)][yPos + (y*s)] = 1;
			myWorld[currentID+4][xPos + (x*s)][yPos + (y*s)] = shipNum;
		}
		return true;
	}
	else {
		return false;
	}
}

/* Placement Helper */
int limit(int val) {
	if (val > 7)
		return 7;
	if (val < 0)
		return 0;
}

/* #END World Initializer Helper */
/*-------------------------------------------------------------------*/



/*-------------------------------------------------------------------*/
/* #START Debug Helper */

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
	}
	printLabeled("xPos", xPosCursor);
	printLabeled("yPos", yPosCursor);
	if (debug)Serial.println("!0######");
}

/* #END Debug Helper */
/*-------------------------------------------------------------------*/

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
	if(debug)Serial.println("Solo oder Versus?");
	setIDandPlayer(0, true);
	chooseMenu(0);
	cursorLocked = true;
	inputCtl(false);
	if (myControlEvent == LEFT) {
		//Play Solo
		chooseMenu(1);
		delay(200);
		if (inputCtl() == CLICK) 
			gameCtl();
	}
	else if (myControlEvent == RIGHT) {
		//Play Versus
		chooseMenu(2);
		delay(200);
		if (inputCtl() == CLICK)
			gameCtl(false);
	}
	else if (myControlEvent == UP) {
		chooseMenu(3);
		delay(200);
		if (inputCtl() == CLICK)
			sound = true;
	}
	else if (myControlEvent == DOWN) {
		chooseMenu(4);
		delay(200);
		if (inputCtl() == CLICK)
			sound = false;
	}

	cursorLocked = false;
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
	/*Display-Demo-Daten*/
	//Anzeige testen....
	if (displayDemo) {
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
	digitalWrite(dPinRegisterOut, LOW); //Schieberegister Ausgang blocken
						   //Daten schieben

	SPI.transfer(0x02);

	byte matrixR = my_red[color];
	byte matrixG = my_green[color];
	byte matrixB = my_blue[color];

	//Weltfarbenmatrix schieben

	SPI.transfer(matrixG);
	if (ctrred == 3) {
		SPI.transfer(matrixR);
	}
	else {
		SPI.transfer(0xFF);
	}
	SPI.transfer(matrixB);


	//Gemeinsame Zeile? näher erläutern
	SPI.transfer(com);
	
	if (!oneMatrixMode || (oneMatrixMode && cursorOnHitMatrix)) {
		matrixR = op_red[color];
		matrixG = op_green[color];
		matrixB = op_blue[color];
	}
	
	//Hitfarbenmatrix schieben
	SPI.transfer(matrixG);
	if (ctrred == 3) {
		SPI.transfer(matrixR);
		ctrred = 0;
	}
	else {
		SPI.transfer(0xFF);
	}
	ctrred++;
	SPI.transfer(matrixB);

	//Ende Daten schieben
	digitalWrite(dPinRegisterOut, HIGH); //Schieberegister Ausgang aktivieren (= neue Anzeige wird geschaltet)

	color++; //naechste Zeile Farben
	if (color >= 8) color = 0; //letzte Zeile Farben -> erste Zeile Farben
	com = 1 << color; //Ansteuerung fuer gemeinsame Zeile
}
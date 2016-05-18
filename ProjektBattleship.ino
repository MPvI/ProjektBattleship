const bool debug = true;

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

/* Game Controller */
void gameCtl(bool solo = true) {
	bool player1 = true;
	bool player2;
	if (solo) {
		Serial.println("I'm in Solo Mode");
		player2 = false;
	}
	else {
		Serial.println("I'm in Versus Mode");
		player2 = true;
	}
	initWorld(0, player1);
	initWorld(1, player2);

	while (worldHasShips(1) && worldHasShips(2))
	{

	}
}

/* World Initializer */
void initWorld(int id, bool player) {
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
		placeShip(id, 4, 2, 2, RIGHT);
		placeShip(id, 4, 2, 2, LEFT);
		placeShip(id, 3, 2, 2, UP);
		placeShip(id, 3, 2, 2, UP);
		placeShip(id, 2, 2, 2, UP);
		placeShip(id, 2, 2, 2, UP);
	}
}

/* Inputwrapper for placing a player ship with placeShip*/
void placePlayerShip(int id, int size) {
	Serial.println("Richtung:");
	cursorLocked = true;
	int dir = inputCtl();
	cursorLocked = false;
	bool placed = false;
	while (!placed) {
		Serial.println("Position:");
		inputCtl(true, id);
		placed = placeShip(id, size, xPosCursor, yPosCursor, dir);
	}
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

	for (int s = 0; s < size; s++)
	{
		if (myWorld[id][xPos + (x*s)][yPos + (y*s)] != 0)
			canPlace = false;
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

//LED Ersatzfunktionen
void printLabeled(String name, int val) {
	Serial.print(name + ": ");
	Serial.println(val);
}

void printWorld(byte world[8][8]) {
	int counter = 0;
	byte worldStream[64]; // TOPLEFT to BOTRIGHT
	if (debug)Serial.println("######8!");
	for (int i = 7; i >= 0; i--)
	{
		for (int j = 0; j < 8; j++)
		{
			if (j == xPosCursor && i == yPosCursor) {
				if (debug)Serial.print("X");
				worldStream[counter] = 110;
			}
			else {
				if (debug)Serial.print(world[j][i]);
				worldStream[counter] = world[j][i];
			}
			counter++;
		}
		if (debug)Serial.println();
	}
	if (debug)Serial.println("!0######");
}

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

/* Setup */
void setup()
{
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
		playMissileSound();
	}
	else if (myControlEvent == DOWN) {
		playMissileSound(true);
	}
}

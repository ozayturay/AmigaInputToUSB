// AmigaInputDevice
//
//  yorgle@gmail.com
//  http://geodesicsphere.blogspot.com
//  http://yorgle.org
//
//  A program for the Arduino Leonardo and other ATmega32u4 based 
//  devices to read in an Amiga Mouse and Joystick and present them
//  as HID USB mouse and keyboard.
//
//  This file should be available through the github repository at
//	https://github.com/BleuLlama/AmigaInputToUSB
//
//  Wiring diagram and such will be available there in image form
//
//  This is distributed under the MIT license.
//  No warranty blah blah blah.


// version history
//
// v 002  2015-07-02  L/R/M mouse buttons, modes, Mouse button fix, simple acceleration
// v 001  2015-07-02  initial version
//

////////////////////////////////////////////////////////////////////////////////
/*
	hookup info (tentative)

	D9 pin	Mouse	Joystick	Arduino Pin

	1	V	Forward		D2
	2	H	Backward	D3
	3	VQ	Left		D4
	4	HQ	Right		D5
	5	M But	n/c		D8

	6	L But	Button 1	D6
	7	+5V			(VCC)
	8	GND			(GND)
	9	R But	Button 2	D7

	Keyboard (TBD)

	NOTE: Also tie D6, D7, D8 to VCC via 10k Ohm resistor (tentative)

*/
////////////////////////////////////////////////////////////////////////////////


// pin configurations
//	Mouse X and Y quadrature pins
#define kMouseXa (3)
#define kMouseXb (5)
#define kMouseYa (2)
#define kMouseYb (4)


//	Mouse buttons
#define kMouseB1 (6)
#define kMouseB2 (7)
#define kMouseB3 (8)

//	Joystick pins
#define kJoyUp    (2)
#define kJoyDown  (3)
#define kJoyLeft  (4)
#define kJoyRight (5)

// on-board LED pin
#define kLED (17)

// usage modes
#define kModeGrayMouse (0) /* amiga mouse quad signals, to HID mouse */
#define kModeJoyMouse  (1) /* joystick sends LRUD, convert to mouse */
#define kModeJoyWASD   (2) /* joystick sends LRUD, convert to Arrow keys */

int mode = kModeGrayMouse;

// ----------------------------------------

// setup - initialize the hardware
void setup() {
  pinMode( kLED, OUTPUT );
  
  // put your setup code here, to run once:
  Serial.begin( 9600 );
  
  initGrayMouse();
  Mouse.begin();
  
  Serial.begin( 9600 );
  
  for( int i=0 ; i<5 ; i++ )
  {
    digitalWrite( kLED, HIGH ); delay( 50 );
    digitalWrite( kLED, LOW ); delay( 50 );
  }
}



// mouse movement history, used for acceleration
char history_x[128];
char history_y[128];
int historyPos=0; // current write position in the history

// initialize for gray code mouse 
void initGrayMouse( void )
{
  // set the mouse and button inputs
  pinMode( kMouseXa, INPUT );
  pinMode( kMouseXb, INPUT );
  pinMode( kMouseYa, INPUT );
  pinMode( kMouseXb, INPUT );
  pinMode( kMouseB1, INPUT_PULLUP );
  pinMode( kMouseB2, INPUT_PULLUP );
  pinMode( kMouseB3, INPUT_PULLUP );
  
  // set the mode
  mode = kModeGrayMouse;
  
  // clear the history
  for( int h=0 ; h<128 ; h++ ) {
    history_x[h] = history_y[h] = 0;
  }
}

// provide a total of all X history values (for acceleration)
int total_x( void )
{
  int ret = 0;
  for( int i=0 ; i < 128 ; i++ )
  {
    ret += history_x[i];
  }
  return ret;
}

// provide a total of all Y history values (for acceleration)
int total_y( void )
{
  int ret = 0;
  for( int i=0 ; i < 128 ; i++ )
  {
    ret += history_y[i];
  }
  return ret;
}


// compare A and B to determine the delta
  // +  00 -> 01 -> 11 -> 10 -> 00 
  // -  00 -> 10 -> 11 -> 01 -> 00
int grayCompare( int a, int b )
{
  a = a & 0x03;
  b = b & 0x03;
  if( a == b ) return 0;
  switch( a ) {
    case( 0x00 ):
      if( b == 0x01) return +1;
      if( b == 0x10) return -1;
      break;
    case( 0x01 ):
      if( b == 0x11) return +1;
      if( b == 0x00) return -1;
      break;
    case( 0x11 ):
      if( b == 0x10) return +1;
      if( b == 0x01) return -1;
      break;
    case( 0x10 ):
      if( b == 0x00) return +1;
      if( b == 0x11) return -1;
      break;
  }
  return 0;
}


// main loop
void loop() {

  switch( mode ) {
    case( kModeGrayMouse ):
      loopGrayMouse();
      break;
    case( kModeJoyMouse ):
    case( kModeJoyWASD ):
    default:
      break;
  }
}

// handler to read the mouse buttons.
// broke this out since it's the same for mouse vs joystick
void handleButtonPresses()
{
  // b1 = left
  // b2 = right
  // b3 = middle
  
  // left
  static int lb1 = LOW;
  int b1 = digitalRead( kMouseB1 );
  
  if( b1 != lb1 ) {
    if( b1 == LOW ) {
      Mouse.press( MOUSE_LEFT );
      delay( 50 ); // fakeo debounce
    }
    if( b1 == HIGH ) Mouse.release( MOUSE_LEFT );
  }
  
  // necessary to prevent mouse fighting
  //if( b1 == HIGH && Mouse.isPressed( MOUSE_LEFT )) {
  //  Mouse.release( MOUSE_LEFT );
  //}
  
  lb1 = b1;

  
  // right
  static int lb2 = LOW;
  int b2 = digitalRead( kMouseB2 );
  
  if( b2 != lb2 ) {
    if( b2 == LOW ) {
      Mouse.press( MOUSE_RIGHT );
      delay( 50 ); // fakeo debounce
    }
    if( b2 == HIGH ) Mouse.release( MOUSE_RIGHT );
  }
  // necessary to prevent mouse fighting
  //if( b2 == HIGH && Mouse.isPressed( MOUSE_RIGHT )) {
  //  Mouse.release( MOUSE_RIGHT );
  //}
  lb2 = b2;
  
  
  // middle
  static int lb3 = LOW;
  int b3 = digitalRead( kMouseB3 );
  
  if( b3 != lb3 ) {
    if( b3 == LOW ) {
      Mouse.press( MOUSE_MIDDLE );
      delay( 50 ); // fakeo debounce
    }
    if( b3 == HIGH ) Mouse.release( MOUSE_MIDDLE );
  }
  // necessary to prevent mouse fighting
  //if( b3 == HIGH && Mouse.isPressed( MOUSE_MIDDLE )) {
  //  Mouse.release( MOUSE_MIDDLE );
  //}
  lb3 = b3;
}

// the main gray mouse loop
void loopGrayMouse()
{
  bool changed = false; // did something change?
  
  // these are static so they persist between calls
  static int x_accum = 0; // accumulated X
  static int y_accum = 0; // accumulated Y

  static int last_hq = 0; // last h quadrature
  static int last_vq = 0; // last v quadrature
  
  // read in the quad/gray code
  int hq = (digitalRead( kMouseXa ) << 1) | digitalRead( kMouseXb );
  int vq = (digitalRead( kMouseYa ) << 1) | digitalRead( kMouseYb );
  
  // check horizontal delta
  if( hq != last_hq ) {
    x_accum += grayCompare( hq, last_hq );
    changed = true;
  }
  last_hq = hq;
  
  // and vertical delta
  if( vq != last_vq ) {
    y_accum += grayCompare( vq, last_vq );
    changed = true;
  }
  last_vq = vq;
  
  
  // generate the acceleration info from gray tick
  historyPos++;
  history_x[ (historyPos & 0x7f) ] = x_accum;
  history_y[ (historyPos & 0x7f) ] = y_accum;
  int tx = total_x() * 2;
  int ty = total_y() * 2;
  
  // if something changed, move the mouse
  if( x_accum || y_accum ) {
    Mouse.move( tx, ty, 0 );
    x_accum = y_accum = 0;
  }
  
  // and check the mouse buttons too
  handleButtonPresses();
}
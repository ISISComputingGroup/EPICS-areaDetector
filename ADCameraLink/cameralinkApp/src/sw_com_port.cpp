/**
 * Class for general serial port. Can be overridden. Used for camera link serial port. 
 *
 *@author Timothy Madden
 *@date 2003
 */
 

#include "sw_com_port.h"

/**
 * construct with name like "COM1"
 * @param   name    C string like "COM2"
 */
 
sw_com_port::sw_com_port(char *name,log_file *lf_)  {
  strcpy(port_name, name);
  is_open = false;
  lf = lf_;
  
  myqueue = new std::queue<unsigned char>(); 
}

/**
 * Destructior. 
 */
 
sw_com_port::~sw_com_port() {
  if (is_open) close();
  delete myqueue;
  
}

/**
 * Set port name like COM1 or etc..
 * @param   n   C string like "COM2"
 */
 
void sw_com_port::setPortName(char *n) { strcpy(port_name, n); }

/**
 * Open comport with baud as int, parity (1,0), nbits 7,8, nstop, 0,1.
 * @param baud      baud rate like 9600 or 115200
 * @param   parity  1 or 0
 * @param  nbits  7 or 8
 * @param   nstop   1 or 0
 */
 
void sw_com_port::open(int baud, int parity, int nbits, int nstop) {
 

  is_open = true;

}

/**
 * Open com port with standard specs and add read time out in ms.
 * @param baud  Baud rate
 * @param   parity  1 or 0
 * @param   nbits   7 or 8 bits
 * @param   nstop   0 opr 1 stop bits
 * @param   rdtimeout int millisec for timeout.
 */
 
void sw_com_port::open(int baud, int parity, int nbits, int nstop,
                       int rdtimeout) {


  is_open = true;

}

/**
 * OPen com port with default settings. 115200 baud, 1stop, no parituy, 8 bit data.
 */
 
void sw_com_port::open(void) {

  is_open = true;


}

/**
 * Write byte to serial port, flush.
 * @param   c   char to write to ser port.
 */
 
void sw_com_port::write(unsigned char c) {
  this->write(&c, 1);

}


/**
 * Write mem buffer of byes, num bytes to ser port, flush.
 * @param   buffer  mem w/ message or data to send to ser port./
 * @param   length  lengt of message to send in bytes
 */
 
void sw_com_port::write(unsigned char *buffer, int length) {
 
 
 
 	for (int k = 0; k<length;k++)
	{
		myqueue->push(buffer[k]);
	}
 
}

/**
 * read 1 byte from ser port. 
 */
 
unsigned char sw_com_port::read(void) {
  unsigned char buffer[10];
  read(buffer, 1);
 
  return (buffer[0]);
}

/**
 * read length bytes into bufferfrom ser port. 
 * @param   buffer  Read ser port into this memory.
 * @param   length  max len of data to read from ser port.
 */
 
void sw_com_port::read(unsigned char *buffer, int length) {


	for (int k=0;k<length;k++)
	{
		if (!myqueue->empty())
		{
		    buffer[k] = myqueue->front();
		     myqueue->pop();
		}   
	}

}
void sw_com_port::flush(void) {
}

/**
 * close ser port. 
 */
 
void sw_com_port::close() {
    is_open = false;
}

/**
 * read ser port until no data left. clears out garbage that may be in serial port. 
 */
 
void sw_com_port::clearPipe(void) {
}


/**
 * Waait in a for loop. Not a sleep. give micro sec.
 * @param  num microsec to loop. or wait.  
 */
 
void sw_com_port::wait(int us) {
  int t0, t1;

  t0 = clock();

  t1 = us * CLOCKS_PER_SEC / 1000000;

  while ((clock() - t0) < t1) {
  }
}

/**
 * Start a stop watch. like tic in matlab.
 */

 void sw_com_port::tic() {
  currenttime = (double)clock() / (double)CLOCKS_PER_SEC;
}

/**
 * read stop watch in sec. return double seconds since tic. let stop watch keep going. 
 * likc toc in matlab.
 */

double sw_com_port::toc() {
  elapsedtime = ((double)clock() / (double)CLOCKS_PER_SEC) - currenttime;
  return (elapsedtime);
}


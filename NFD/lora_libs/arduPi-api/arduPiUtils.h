/*! \file arduPiUtils.h
    \brief Library containing useful general functions
    
    Copyright (C) 2015 Libelium Comunicaciones Distribuidas S.L.
    http://www.libelium.com
 
    This program is free software: you can redistribute it and/or modify
    it under the terms of the GNU Lesser General Public License as published by
    the Free Software Foundation, either version 2.1 of the License, or
    (at your option) any later version.
   
    This program is distributed in the hope that it will be useful,
    but WITHOUT ANY WARRANTY; without even the implied warranty of
    MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
    GNU Lesser General Public License for more details.
  
    You should have received a copy of the GNU Lesser General Public License
    along with this program.  If not, see <http://www.gnu.org/licenses/>.
  
    Version:		1.3
    Design:			David Gascon
    Implementation:	Alberto Bielsa, David Cuartielles, Ruben Martin
                        Ahmad Saad  
*/
  
  
/*! \def arduPiutils_h
    \brief The library flag
    
 */
#ifndef arduPiUtils_h
#define arduPiUtils_h

/******************************************************************************
 * Includes
 ******************************************************************************/
 
#include <inttypes.h>
#include "arduPiClasses.h"


/******************************************************************************
 * Definitions & Declarations
 ******************************************************************************/

/*! \def SOCKET_PW
    \brief Module Power pin
*/
#define SOCKET_PW 3

/* MUX_SOCKET0_nSOCKET1
 * Multiplexor input selector pin
*/
#define MUX_SOCKET0_nSOCKET1         5


/******************************************************************************
 * Class
 ******************************************************************************/
 
//! arduPiUtils Class
/*!
	arduPiUtils Class defines useful general functions
 */
class arduPiUtils
{
  private:
  
  public:

  //! class constructor
  /*!
  It does nothing
  \param void
  \return void
  */
  arduPiUtils(void);

  //////////////////////////////////////////////////
  //        Embebed Socket Power HANDLED          //
  //////////////////////////////////////////////////
  //! It power ON the module in socket
  /*!  
  \return void
   */
  void socketON();

  //! It power OFF the module in socket
  /*!  
  \return void
   */
  void socketOFF();


  //////////////////////////////////////////////////
  //Support for "Multiprotocol Radio Shield v1.0" //
  //////////////////////////////////////////////////
  //! It sets multiplexer on UART_0 to SOCKET0
  /*!  
  \return void
   */
  void setMuxSocket0();

  //! It sets multiplexer on UART_0 to SOCKET1
  /*!  
  \return void
   */
  void setMuxSocket1();

  //! It sets multiplexer on UART_0 to default
  /*!  
  \return void
   */
  void setMuxUSB();

  //////////////////////////////////////////////////
  //Support for "Multiprotocol Radio Shield v2.0" //
  //////////////////////////////////////////////////
  //! Initialize the MCP230008 expansor
  /*!  
  \return void
   */
  void multiprotocolBegin();
  //! Disable Multiplexor
  /*!  
  \return void
   */
  void disableMUX();


  // Socket0 management
  //! Set Multiplexor UART point to Socket0
  /*!  
  \return void
   */
  void setMUXSocket0();
  //! Set Power ON Socket0
  /*!  
  \return void
   */
  void setONSocket0();
  //! Set Power OFF Socket0
  /*!  
  \return void
   */
  void setOFFSocket0();
  //! Set Chip Select in Socket0
  /*!  
  \return void
   */
  void setCSSocket0();
  //! UnSet Chip Select in Socket0
  /*!  
  \return void
   */
  void unsetCSSocket0();


  // Socket1 management
  //! Set Multiplexor UART point to Socket1
  /*!  
  \return void
   */
  void setMUXSocket1();
  //! Set Power ON Socket1
  /*!  
  \return void
   */
  void setONSocket1();
  //! Set Power OFF Socket1
  /*!  
  \return void
   */
  void setOFFSocket1();
  //! Set Chip Select in Socket1
  /*!  
  \return void
   */
  void setCSSocket1();
  //! UnSet Chip Select in Socket1
  /*!  
  \return void
   */
  void unsetCSSocket1();
  
  //Socket internal state
  uint8_t socket0_state;
  uint8_t socket1_state;    


  //////////////////////////////////////////////////
  // String functions HANDLED                     //
  //////////////////////////////////////////////////
  //! It converts a hexadecimal number stored in an array to a string (8 Byte numbers)
  /*!
  \param uint8_t* number : hexadecimal array to conver to a string
  \param const char* macDest : char array where the converted number is stored
  \param uint8_t length : length to copy
  \return void
  \sa long2array(long num, char* numb), str2hex(char* str), str2hex(uint8_t* str)
   */
  void hex2str(uint8_t* number, char* macDest, uint8_t length);

  //! It converts a float into a string
  /*!
  \param float fl : the float to convert
  \param char str[] : the string where store the float converted
  \param int N : the number of decimals
  \return void
   */
  void float2String(float fl, char str[], int N);

    
};

extern arduPiUtils Utils;

#endif




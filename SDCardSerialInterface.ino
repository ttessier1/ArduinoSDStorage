

/*
  SD card file dump

  This example shows how to read a file from the SD card using the
  SD library and send it over the serial port.

  The circuit:
   SD card attached to SPI bus as follows:
 ** MOSI - pin 11
 ** MISO - pin 12
 ** CLK - pin 13
 ** CS - pin 4 (for MKRZero SD: SDCARD_SS_PIN)

  created  22 December 2010
  by Limor Fried
  modified 9 Apr 2012
  by Tom Igoe

  This example code is in the public domain.

*/

#include <SPI.h>
#include <SD.h>
#include <SHA256.h>


#define MAX_PATH 260
char path[MAX_PATH];
char newPath[MAX_PATH];

void SerialPrintHex(uint8_t value);
SHA256 sha256;
#define HASH_SIZE 32
#define BUFFER_SIZE 64
byte buffer[BUFFER_SIZE];
uint8_t result[HASH_SIZE];

const int chipSelect = 4;

void setup() {
  // Open serial communications and wait for port to open:
  Serial.begin(2000000);

  memset(path,0,sizeof(path));
  memset(newPath,0,sizeof(newPath));

  while (!Serial) {
    ; // wait for serial port to connect. Needed for native USB port only
  }


  Serial.print("InitSD...");

  // see if the card is present and can be initialized:
  if (!SD.begin(chipSelect)) {
    Serial.print("FAIL\r\n");
    // don't do anything more:
    while (1){
      
    }
  }
  Serial.print("OK\r\n");

  // open the file. note that only one file can be open at a time,
  // so you have to close this one before opening another.
  //File dataFile = SD.open("datalog.txt");

  // if the file is available, write to it:
  /*if (dataFile) {
    while (dataFile.available()) {
      Serial.write(dataFile.read());
    }
    dataFile.close();
  }
  // if the file isn't open, pop up an error:
  else {
    Serial.println("error opening datalog.txt");
  }*/
  strcpy(path,"/");
}

// commands
// 1 - list directories
// 2 - change directory
// 3 - 

enum state
{
  state_initial,
  state_read_ping,
  state_read_list,
  state_read_change_dir,
  state_read_delete_file,
  state_read_create_file,
  state_read_download_file,
  state_read_get_size,
  state_read_get_hash,
  state_invalid,
};

state serialState = state_initial ;

void loop() {
  static uint16_t index;
  static char lastChar;
  uint64_t filesize;
  char inChar ;
  int bytesRead = 0 ;
  if(Serial.available()>0)
  {
    inChar = Serial.read();
    switch(serialState)
    {
      case state_initial:
        switch(inChar)
        {
          case '0':
            serialState = state_read_ping;
            index= 0;
            newPath[index]=inChar;
            newPath[index+1]='\0';
            index++;
          break;
          case '1': // list files
            serialState = state_read_list;
            index= 0;
            newPath[index]=inChar;
            newPath[index+1]='\0';
            index++;
          break;
          case '2': // Change Directories
            serialState = state_read_change_dir;
            index= 0;
            // read next directory chars
          break;
          case '3':// Delete File
            serialState = state_read_delete_file;
            index= 0;
          break;
          case '4': //upload file
            serialState = state_read_create_file;
            index= 0;
          break;
          case '5': // download file
            serialState = state_read_download_file;
            index= 0;
          break;
          case '6': // get file size
            serialState = state_read_get_size;
            index= 0;
          break;
          case '7': // get file hash
            serialState = state_read_get_hash;
            index= 0;
          break;
          case '\r':
          case '\n':
          break;
          default:
            serialState = state_invalid;
            Serial.print("Unrecognized Command[");
            Serial.print(inChar);
            Serial.print("]\r\n");
        }
      break;
      case state_read_ping:
        newPath[index]=inChar;
        newPath[index+1]='\0';
        if(strcmp(newPath,"0\r\n")==0)
        {
          
          // do ping response
          Serial.print("ping\r\n");
          serialState = state_initial;
          index = 0;
        }
        else
        {
          if(lastChar=='\r' && inChar =='\n')
          {
            Serial.print("BAD Read:[");
            Serial.print(newPath);
            Serial.print("] ping\r\n");
            index=0;
            serialState = state_initial;
          }
          else if ( index > 3)
          {
            serialState = state_invalid;
          }
          else
          {
            index++;
          }
        }
      break;
      case state_read_list:
        newPath[index]=inChar;
        if(memcmp(newPath,"1\r\n",strlen("1\r\n"))==0)
        {
          File root = SD.open(path,FILE_READ);
          printDirectory(root,0);
          Serial.write("\0\r\n",3);
          serialState = state_initial;
        }
        else
        {
          if(lastChar=='\r' && inChar =='\n')
          {
            Serial.print("BAD Read list\r\n");
            Serial.print("\0\r\n");
            index=0;
            serialState = state_initial;
          }
          else if ( index > 3)
          {
            serialState = state_invalid;
          }
          else
          {
            index++;
          }
        }
      break;
      case state_read_change_dir:

      break;
      case state_read_delete_file:

      break;
      case state_read_create_file:

      break;
      case state_read_download_file:
        if(inChar!='\r' && inChar != '\n')
        {
          newPath[index]=inChar;
          newPath[index+1]='\0';
        }
        if(lastChar=='\r' && inChar =='\n')
        {
          if(SD.exists(newPath))
          {
            File theFile = SD.open(newPath);
            if(!theFile.isDirectory())
            {
              Serial.print("File Follows\r\n");
              while(bytesRead==BUFFER_SIZE)
              {
                bytesRead = theFile.read(buffer,BUFFER_SIZE);
                for(int i = 0 ; i < bytesRead; i++)
                {
                  SerialPrintHex(buffer[i]); // print everything in hex
                }
                Serial.print("\r\n");
              }
              Serial.print("\r\n");// end of file is two empty lines
              theFile.close();
            }
            else
            {
              filesize = 0;
              theFile.close();
              Serial.print("File is directory\r\n\r\n");
            }
          }
          else
          {
            filesize = -1;
            Serial.print("File Not Exists\r\n\r\n");
          }
          index=0;
          serialState = state_initial;
        }
        else if ( index >= MAX_PATH)
        {
          Serial.print("Buffer Overflow\r\n\r\n");
          serialState = state_invalid;
        }
        else
        {
          if(inChar!='\r' && inChar != '\n')
          {
            index++;
          }
        }
      break;
      case state_read_get_size:
        if(inChar!='\r' && inChar != '\n')
        {
          newPath[index]=inChar;
          newPath[index+1]='\0';
        }
        if(lastChar=='\r' && inChar =='\n')
        {
          if(SD.exists(newPath))
          {
            File theFile = SD.open(newPath);
            if(!theFile.isDirectory())
            {
              
              filesize = theFile.size();
              theFile.close();
              Serial.print(filesize,HEX);
              Serial.print("\r\n");
            }
            else
            {
              filesize = 0;
              theFile.close();
              Serial.print(filesize,HEX);
              Serial.print("\r\n");
            }
          }
          else
          {
            filesize = -1;
            Serial.print(filesize,HEX);
            Serial.print("\r\n");
          }
          index=0;
          serialState = state_initial;
        }
        else if ( index >= MAX_PATH)
        {
          serialState = state_invalid;
        }
        else
        {
          if(inChar!='\r' && inChar != '\n')
          {
            index++;
          }
        }
      break;
      case state_read_get_hash:
        if(inChar!='\r' && inChar != '\n')
        {
          newPath[index]=inChar;
          newPath[index+1]='\0';
        }
        if(lastChar=='\r' && inChar =='\n')
        {
          if(SD.exists(newPath))
          {
            File theFile = SD.open(newPath);
            if(theFile)
            {
              if(!theFile.isDirectory())
              {
                sha256.reset();
                while (theFile.available()) {
                  Serial.print(".\r\n");
                  bytesRead = theFile.read(buffer,BUFFER_SIZE);
                  sha256.update(buffer,bytesRead);
                }
                sha256.finalize(result,HASH_SIZE);
                for(int i = 0 ; i < HASH_SIZE; i++)
                {
                  SerialPrintHex(result[i]);
                }
                Serial.print("\r\n");
                theFile.close();
              }
              else
              {
                Serial.print("000000000000000000000000000000000000000000000000000000000000000\r\n");
              }
              
                

            }
            else
            {
              filesize = 0;
              Serial.print("FFFFFFFFFFFFFFFFFFFFFFFFFFFFFFFFFFFFFFFFFFFFFFFFFFFFFFFFFFFFFFFF\r\n");
            }
          }
          else
          {
            filesize = -1;
            Serial.print("FFFFFFFFFFFFFFFFFFFFFFFFFFFFFFFFFFFFFFFFFFFFFFFFFFFFFFFFFFFFFFFF\r\n");
          }
          index=0;
          serialState = state_initial;
        }
        else if ( index >= MAX_PATH)
        {
          serialState = state_invalid;
        }
        else
        {
          if(inChar!='\r' && inChar != '\n')
          {
            index++;
          }
        }
      break;
      case state_invalid:
        // wait until new line to reset state
        if(lastChar=='\r' && inChar =='\n')
        {
          Serial.print("BAD Read\r\n");
          index=0;
          serialState = state_initial;
        }
      break;
      default:



      break;
    }
    lastChar=inChar;
  }
}

void printDirectory(File dir, int numTabs) {
  while (true) {

    File entry =  dir.openNextFile();
    if (! entry) {
      // no more files
      break;
    }
    //for (uint8_t i = 0; i < numTabs; i++) {
    //  Serial.print('\t');
   // }
    Serial.print(entry.name());
    Serial.print("\r\n");
    /*if (entry.isDirectory()) {
      Serial.println("/");
      printDirectory(entry, numTabs + 1);
    } else {
      // files have sizes, directories do not
      Serial.print("\t\t");
      Serial.println(entry.size(), DEC);
    }*/
    entry.close();
  }
}

void SerialPrintHex(uint8_t value)
{
  if(value <16)
  {
    Serial.print("0");
    Serial.print(value,HEX);
  }
  else
  {
    Serial.print(value,HEX);
  }
}
/*This header file contains helpers and definitions pertaining to the meshcore packet structure, and protocol handeling.
* The meshcore format uses little-endian notation, with bit-0 being the right-most and bit-7 being the leftmost.
*/
#pragma once

//Header Contents
//A V1 Meshcore header consists of 8 bits, with 0-1 representing route type, 2-5 representing payload type, and 6-7 representing payload version.

//Route Type
enum route : uint8_t {
  TRANSPORT_FLOOD     = 0b00, //Flood Routing + Transport Codes
  FLOOD               = 0b01, //Flood Routing
  DIRECT              = 0b10, //Direct Routing
  TRANSPORT_DIRECT    = 0b11  //Direct Routing + Transport Codes
};

enum payloadType : uint8_t {
  REQUEST             = 0b0000, //Request destination / source hashes + MAC
  RESPONSE            = 0b0001, //Response to REQUEST or ANON_REQUEST
  TEXT_MSG            = 0b0010, //Plain Text Message
  ACK                 = 0b0011, //Acknowledgment
  ADVERT              = 0b0100, //Node Advertisement
  GROUP_TEXT          = 0b0101, //Group Text Message
  GROUP_DATA          = 0b0110, //Group Datagram
  ANON_REQUEST        = 0b0111, //Anonymous Request
  PATH                = 0b1000, //Returned Path
  TRACE               = 0b1001, //Trace a path collecting SNR for each hop.
  MULTIPART           = 0b1010, //Packet is part of a sequence of packets
  CONTROL             = 0b1011, //Control packet data
  RESERVED_1          = 0b1100, //Reserved
  RESERVED_2          = 0b1101, //Reserved
  RESERVED_3          = 0b1110, //Reserved
  RAW_CUSTOM          = 0b1111  //Custom packet (raw bytes, custom encryption)
};

enum version : uint8_t {
  V1                  = 0b00, //V1 1-Byte src/dest hshes, 2 byte MAC
  V2                  = 0b01, //V2 Future Version (2-Byte hashes, 4 Byte MAC)
  V3                  = 0b10, //Future Version
  V4                  = 0b11  //Future Version
};

struct packetHeader {
  uint8_t route : 2;
  uint8_t payloadType : 4;
  uint8_t payloadVersion : 2;
};

/*Packet Header Usage Example
* PacketHeader header = {0};
* header.route = TRANSPORT_FLOOD
* header.payloadType = TEXT_MSG;
* header.version = V1;
*
*Reading Header Values
* if (header.route == FLOOD) {
  //Handle flood routing
}
*/

//Transport Codes ---------------------------- Not Implemented Yet :)

MAX_PATH_SIZE = uint8_t 0b1111; //(64) //Maximum allowable number of hops.

// Path Length 1- Byte, Bits 0-5 store path hash count / hop count e.g 0-63, bits 6-7 store path hash size minus 1

uint8_t pathHopCount = 0b00000; //Up to 64 hops.

enum pathHashSize : uint8_t {
  one_byte      = 0b00,   //One Byte Path Hash
  two_byte      = 0b01,   //Two Byte Path Hash
  three_byte    = 0b10,   //Three Byte Path Hash
  reserved      = 0b11    //Unsupported
};

//Path Length Encoding Example
// 0x00   zero-hop packet, no path bytes
// 0x05 = 0b00000101 five hops using 1 byte hashes, so path is 5 bytes long
// 0x45 = 0b01000101 five hops using 2 byte hashes, so path is 10 bytes long

//Path - Used for direct routing or flood path tracking. Up to a maximum of 64 bytes defined by MAX_PATH_SIZE.
//Effective byte length is calculated from the encoded hop count and hash size and not taken directly from path_length the calculation is pathHopCount * path_Hash_Size bytes long

//Payload, maximum of 184 bytes defined by MAX_PACKET_PAYLOAD.
uint8_t MAX_PACKET_PAYLOAD = 184;

//Example packet.
// 0b[PAYLOAD]00001000010000001000


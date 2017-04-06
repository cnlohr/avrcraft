#ifndef _DUMBUTILS_H
#define _DUMBUTILS_H

#include <stdint.h>

void SpawnEntity( uint16_t eid, uint8_t type, uint16_t x, uint16_t y, uint16_t z );
void EntityUpdatePos( uint16_t entity, uint16_t x, uint16_t y, uint16_t z, uint8_t yaw, uint8_t pitch );

void UpdateSlot( uint8_t window, uint8_t slot, uint8_t count, uint16_t id, uint8_t damage );
void GPChat( const char * text );

//Update a sign at a specific location with a string and a numerical value.
void SignUp( uint8_t x, uint8_t y, uint8_t z, const char* st, uint8_t val );
void SignTextUp( uint8_t x, uint8_t y, uint8_t z, const char * line1, const char * line2 );

//Update a block at a given x, y, z (good for 0..255 in each dimension)
void SblockInternal( uint8_t x, uint8_t y, uint8_t z, uint8_t bt, uint8_t meta );

//Used for sending a position triplet.
void InternalSendPosition (uint8_t x, uint8_t y, uint8_t z );


#endif


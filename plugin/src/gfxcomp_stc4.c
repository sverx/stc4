#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <stdbool.h>

typedef unsigned char uint8_t;
typedef unsigned int  uint32_t;

__declspec(dllexport) const char* getName() {
	return "Simple Tile Compressor Four";
}

__declspec(dllexport) const char* getExt() {
	return "stc4compr";
}

#define MAX_GROUP_REPETITIONS   31
#define GROUP_MATCHES_MASK      0x20
#define INV_GROUP_MATCHES_MASK  0x30

__declspec(dllexport) int compressTiles(const uint8_t* pSource, const uint32_t numTiles, uint8_t* pDestination, const uint32_t destinationLength) {

  if (destinationLength<(numTiles*40+1))
    return (0);                 // please give me more space for the data (up to 40 bytes per tile needed worst case scenario, plus 1 byte terminator)

  unsigned int tilenum;
  unsigned char i,j,mask,lastvalue;
  bool lastvalid;
  unsigned char values[4];
  unsigned char valuescount;
  unsigned int finalsize=0;

  // stc4 buffer variables
  unsigned char buffer[4];
  unsigned char group_repetitions=0;
  unsigned char group_unmatches,group_unmatches_mask;

  // test
  //~ unsigned char inv_group_unmatches,inv_group_unmatches_mask;

  for (tilenum=0;tilenum<numTiles;tilenum++) {  // loop over all tiles
    for (i=0;i<8;i++,pSource+=4) {              // loop over all the 8 rows of the tile

      /* handle group repetitions */
      if ((tilenum+i)>0) {                      // of course we need to exclude the absolute first iteration

        if (memcmp(&buffer,pSource,4)==0) {     // if these 4 bytes are same as buffered 4

          group_repetitions++;

          if (group_repetitions==MAX_GROUP_REPETITIONS) {   // dump if reached 31 repetitions, we can't have more than that

            *pDestination++=group_repetitions;
            finalsize++;
            group_repetitions=0;

          }
          continue;                             // this is because we already processed these 4 bytes

        } else {                                // these 4 bytes are NOT same as previous 4, but we might have been in a run already...

          if (group_repetitions>0) {

            *pDestination++=group_repetitions;
            finalsize++;
            group_repetitions=0;

          }

        }

      }

      /* handle single group of bytes */
      mask=0b00000000;
      lastvalid=false;
      valuescount=0;

      if (pSource[0]==0x00)             mask|=0b10000000;
      else if (pSource[0]==0xFF)        mask|=0b11000000;
      else {
                                        mask|=0b01000000;  // raw
        lastvalid=true;
        lastvalue=pSource[0];
        values[valuescount++]=pSource[0];
      }

      if (pSource[1]==0x00)             mask|=0b00100000;
      else if (pSource[1]==0xFF)        mask|=0b00110000;
      else if ((lastvalid==false) ||
               (pSource[1]!=lastvalue)) {
                                        mask|=0b00010000;  // raw
        lastvalid=true;
        lastvalue=pSource[1];
        values[valuescount++]=pSource[1];
      }

      if (pSource[2]==0x00)             mask|=0b00001000;
      else if (pSource[2]==0xFF)        mask|=0b00001100;
      else if ((lastvalid==false) ||
               (pSource[2]!=lastvalue)) {
                                        mask|=0b00000100;  // raw
        lastvalid=true;
        lastvalue=pSource[2];
        values[valuescount++]=pSource[2];
      }

      if (pSource[3]==0x00)             mask|=0b00000010;
      else if (pSource[3]==0xFF)        mask|=0b00000011;
      else if ((lastvalid==false) ||
               (pSource[3]!=lastvalue)) {
                                        mask|=0b00000001;  // raw
        lastvalid=true;
        lastvalue=pSource[3];
        values[valuescount++]=pSource[3];
      }


      // compare the current 4 bytes with the buffer to see if SOME match, and see if it's better to use that compression instead of what already calculated
      // note that it's even useless in case only 1 byte or 2 bytes has to be written anyway, because stc0 decompression has to be preferred as it's faster

      if (((tilenum+i)>0) && (valuescount>=2)) {  // of course we need to exclude the absolute first iteration, and let's do this only if it's worthy

        group_unmatches=0;
        group_unmatches_mask=0;
        for (j=0;j<4;j++) {                       // check how many NOT matches we have
          if (buffer[j]!=pSource[j]) {
            group_unmatches++;
            group_unmatches_mask|=(1<<(3-j));
          }
        }

        //~ inv_group_unmatches=0;
        //~ inv_group_unmatches_mask=0;
        //~ for (j=0;j<4;j++) {                       // check how many NOT inverted matches we have
          //~ if (buffer[j]!=(~pSource[j])) {
            //~ inv_group_unmatches++;
            //~ inv_group_unmatches_mask|=(1<<(3-j));
          //~ }
        //~ }

        // use inverted match only if better than regular matches
        //~ if (inv_group_unmatches<group_unmatches)  {

          //~ if (inv_group_unmatches<valuescount) {        // pick the inverted group match only if it's the best option (if it saves compared to previous method)

            //~ *pDestination++=(INV_GROUP_MATCHES_MASK|group_unmatches_mask);   // write group mask byte
            //~ finalsize++;

            //~ for (j=0;j<4;j++) {                     // write the mismatching bytes
              //~ if (buffer[j]!=(~pSource[j])) {
                //~ *pDestination++=pSource[j];
                //~ finalsize++;
              //~ }
            //~ }

            //~ memcpy(&buffer,pSource,4);              // copy these last 4 bytes to the buffer

            //~ continue;                               // this is because we already processed these 4 bytes
          //~ }

        //~ } else {

          if (group_unmatches<valuescount) {        // pick the group match only if it's the best option (if it saves compared to previous method)

            *pDestination++=(GROUP_MATCHES_MASK|group_unmatches_mask);   // write group mask byte
            finalsize++;

            for (j=0;j<4;j++) {                     // write the mismatching bytes
              if (buffer[j]!=pSource[j]) {
                *pDestination++=pSource[j];
                finalsize++;
              }
            }

            memcpy(&buffer,pSource,4);              // copy these last 4 bytes to the buffer

            continue;                               // this is because we already processed these 4 bytes

          }
        //~ }

      }


      /* handle single group of bytes because everything else failed */

      memcpy(&buffer,pSource,4);        // copy these 4 bytes to the buffer

      *pDestination++=mask;             // write mask byte

      for (j=0;j<valuescount;j++)
        *pDestination++=values[j];      // dump uncompressed values

      finalsize+=(1+valuescount);       // add this to finalsize
    }
  }

  /* tiles are finished, check if we still have to dump a group repetition run */
  if (group_repetitions>0) {

    *pDestination++=group_repetitions;
    finalsize++;

  }

  *pDestination=0x00;                   // end of data marker
  finalsize++;

  return (finalsize);                   // report size to caller
}

/*
  Teem: Tools to process and visualize scientific data and images             .
  Copyright (C) 2013, 2012, 2011, 2010, 2009  University of Chicago
  Copyright (C) 2008, 2007, 2006, 2005  Gordon Kindlmann
  Copyright (C) 2004, 2003, 2002, 2001, 2000, 1999, 1998  University of Utah

  This library is free software; you can redistribute it and/or
  modify it under the terms of the GNU Lesser General Public License
  (LGPL) as published by the Free Software Foundation; either
  version 2.1 of the License, or (at your option) any later version.
  The terms of redistributing and/or modifying this software also
  include exceptions to the LGPL that facilitate static linking.

  This library is distributed in the hope that it will be useful,
  but WITHOUT ANY WARRANTY; without even the implied warranty of
  MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the GNU
  Lesser General Public License for more details.

  You should have received a copy of the GNU Lesser General Public License
  along with this library; if not, write to Free Software Foundation, Inc.,
  51 Franklin Street, Fifth Floor, Boston, MA  02110-1301  USA
*/

#include "teem/nrrd.h"
#include <testDataPath.h>
#include <math.h>
#include <stdlib.h>

/*
** Tests:
** Streaming Load and Save of a nrrd file to/from multiple arrays.
*/

int
main(int argc, const char **argv) {
  const char *me;
  Nrrd *nin;
  airArray *mop;
  char *fullname;

  AIR_UNUSED(argc);
  me = argv[0];
  mop = airMopNew();

  nin = nrrdNew();
  airMopAdd(mop, nin, (airMopper)nrrdNuke, airMopAlways);
  fullname = testDataPathPrefix("fmob-c4h.nrrd");
  airMopAdd(mop, fullname, airFree, airMopAlways);

  // Load the header and set header parameters in nin
  NrrdIoState *nioReadHeader = nrrdIoStateNew();
  if (nioReadHeader == NULL) {
    fprintf(stderr, "%s: could not allocate NRRD Teem I/O struct \n", me);
    return 1;
  }

  nioReadHeader->skipData = 1;

  if (nrrdLoad(nin, fullname, nioReadHeader)) {
    char *err;
    airMopAdd(mop, err = biffGetDone(NRRD), airFree, airMopAlways);
    fprintf(stderr, "%s: trouble reading data \"%s\":\n%s",
            me, fullname, err);
    airMopError(mop); return 1;
  }

  free(nioReadHeader);

  // Create the arrays where to stream the data
  long int numberOfVolumeElement = nrrdElementNumber(nin);
  long int numberOfImages = 5;
  long int numberOfVolumeElementPerImage = (long int) numberOfVolumeElement / numberOfImages;
  double *dataDest1 = malloc(numberOfVolumeElementPerImage*sizeof(*dataDest1));
  double *dataDest2 = malloc(numberOfVolumeElementPerImage*sizeof(*dataDest2));
  double *dataDest3 = malloc(numberOfVolumeElementPerImage*sizeof(*dataDest3));
  double *dataDest4 = malloc(numberOfVolumeElementPerImage*sizeof(*dataDest4));
  double *dataDest5 = malloc(numberOfVolumeElementPerImage*sizeof(*dataDest5));

  // Read in nin->data in chunks
  long int numberOfElementsPerChunk = pow(10, 3);
  long int numberOfChunks = (long int) floor(numberOfVolumeElementPerImage / numberOfElementsPerChunk);
  long int numberOfElementsPerFinalChunk = numberOfVolumeElementPerImage - (numberOfChunks * numberOfElementsPerChunk);
  long int elementSize = nrrdElementSize(nin);

  // Read the images
  for (long int imageIndex = 0; imageIndex < numberOfImages; imageIndex++) {
    // Read the chunk : each image needs to streamed as well in chunks
    // because nrrdLoad make a copy of teh read data in nin->data
    for (long int chunkIndex = 0; chunkIndex <= numberOfChunks; ++chunkIndex) {
      NrrdIoState *nioReadTemp = nrrdIoStateNew();
      if (nioReadTemp == NULL) {
        fprintf(stderr, "%s: could not allocate NRRD Teem I/O struct \n", me);
        return 1;
      }

      nioReadTemp->chunkStartElement = imageIndex * numberOfVolumeElementPerImage +
                                       chunkIndex * numberOfElementsPerChunk;
      if (chunkIndex < numberOfChunks) {
        nioReadTemp->chunkElementCount = numberOfElementsPerChunk;
      } else {
        nioReadTemp->chunkElementCount = numberOfElementsPerFinalChunk;
      }

      if (nrrdLoad(nin, fullname, nioReadTemp)) {
        char *err;
        airMopAdd(mop, err = biffGetDone(NRRD), airFree, airMopAlways);
        fprintf(stderr, "%s: trouble reading data \"%s\":\n%s",
                me, fullname, err);
        airMopError(mop); return 1;
      }

      if (nin->data == NULL) {
        fprintf(stderr, "%s: could not allocate nin data pointer \n", me);
        return 1;
      }

      long int Chunksize = nioReadTemp->chunkElementCount * elementSize;

      if (imageIndex == 0) {
        memcpy(dataDest1 + (chunkIndex * numberOfElementsPerChunk), nin->data, Chunksize);
      } else if (imageIndex == 1){
        memcpy(dataDest2 + (chunkIndex * numberOfElementsPerChunk), nin->data, Chunksize);
      } else if (imageIndex == 2){
        memcpy(dataDest3 + (chunkIndex * numberOfElementsPerChunk), nin->data, Chunksize);
      } else if (imageIndex == 3){
        memcpy(dataDest4 + (chunkIndex * numberOfElementsPerChunk), nin->data, Chunksize);
      } else if (imageIndex == 4){
        memcpy(dataDest5 + (chunkIndex * numberOfElementsPerChunk), nin->data, Chunksize);
      }

      // Free the nrrd->data but don't touch nrrd struct
      nin->data = airFree(nin->data);
      free(nioReadTemp);
    }
  }

  // Stream array to file
  Nrrd *ncopy = nrrdNew();
  nrrdCopy(ncopy, nin);
  NrrdIoState *nioWrite = nrrdIoStateNew();
  nioWrite->chunkElementCount = numberOfVolumeElementPerImage;
  nioWrite->keepNrrdDataFileOpen = 0;
  // Save the images
  for (long int imageIndex = 0; imageIndex < numberOfImages; imageIndex++) {
    if (imageIndex > 0) {
      nioWrite->keepNrrdDataFileOpen = 1;
    }

    // In writing. We can give to nrrdSave directly the pointer to teh data.
    // Therefore we don't need to stream each image to ncopy->data in chunks.
    if (imageIndex == 0) {
      ncopy->data = dataDest1;
    } else if (imageIndex == 1){
      ncopy->data = dataDest2;
    } else if (imageIndex == 2){
      ncopy->data = dataDest3;
    } else if (imageIndex == 3){
      ncopy->data = dataDest4;
    } else if (imageIndex == 4){
      ncopy->data = dataDest5;
    }

    nioWrite->chunkStartElement = numberOfVolumeElementPerImage * imageIndex;

    if (nrrdSave("tloadTest.nrrd", ncopy, nioWrite)) {
      char *err;
      airMopAdd(mop, err = biffGetDone(NRRD), airFree, airMopAlways);
      fprintf(stderr, "%s: trouble w/ save :\n%s\n", me, err);
      airMopError(mop); return 1;
      }
  }

  // Free the nrrd struct but don't touch nrrd->data
  ncopy = nrrdNix(ncopy);
  nioWrite = nrrdIoStateNix(nioWrite);

  // Load the full array (no streaming)
  if (nrrdLoad(nin, "tloadTest.nrrd", NULL)) {
    char *err;
    airMopAdd(mop, err = biffGetDone(NRRD), airFree, airMopAlways);
    fprintf(stderr, "%s: trouble reading data \"%s\":\n%s",
            me, fullname, err);
    airMopError(mop); return 1;
  }

  // Compare dataDest with nin->data
  double *data_d = (double*)(nin->data);
  double eps = 0.000001;
  for (long int ii = 0; ii < numberOfVolumeElementPerImage; ++ii) {
     if (*(dataDest1 + ii) - *(data_d + ii) > eps) {
       fprintf(stderr, "%s: streamed array and not streamed array are different! \n", me);
       return 1;
     }
  }
  for (long int ii = 0; ii < numberOfVolumeElementPerImage; ++ii) {
     if (*(dataDest2 + ii) - *(data_d + ii + numberOfVolumeElementPerImage) > eps) {
       fprintf(stderr, "%s: streamed array and not streamed array are different! \n", me);
       return 1;
     }
  }
  for (long int ii = 0; ii < numberOfVolumeElementPerImage; ++ii) {
     if (*(dataDest3 + ii) - *(data_d + ii + (numberOfVolumeElementPerImage * 2)) > eps) {
       fprintf(stderr, "%s: streamed array and not streamed array are different! \n", me);
       return 1;
     }
  }
  for (long int ii = 0; ii < numberOfVolumeElementPerImage; ++ii) {
     if (*(dataDest4 + ii) - *(data_d + ii + (numberOfVolumeElementPerImage * 3)) > eps) {
       fprintf(stderr, "%s: streamed array and not streamed array are different! \n", me);
       return 1;
     }
  }
  for (long int ii = 0; ii < numberOfVolumeElementPerImage; ++ii) {
     if (*(dataDest5 + ii) - *(data_d + ii + (numberOfVolumeElementPerImage * 4)) > eps) {
       fprintf(stderr, "%s: streamed array and not streamed array are different! \n", me);
       return 1;
     }
  } 

  free(dataDest1);
  free(dataDest2);
  free(dataDest3);
  free(dataDest4);
  free(dataDest5);
  airMopOkay(mop);
  return 0;
}

/*
  Teem: Tools to process and visualize scientific data and images
  Copyright (C) 2006, 2005  Gordon Kindlmann
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

  You should have received a copy of the GNU Lesser General Public
  License along with this library; if not, write to the Free Software
  Foundation, Inc., 59 Temple Place, Suite 330, Boston, MA  02111-1307  USA
*/

#include "pull.h"
#include "privatePull.h"

char
_pullInfoStr[][AIR_STRLEN_SMALL] = {
  "(unknown pullInfo)",
  "ten",
  "teni",
  "hess",
  "in",
  "ingradvec",
  "hght",
  "hghtgradvec",
  "hghthessian",
  "seedthresh",
  "tan1",
  "tan2",
  "tanmode",
  "isoval",
  "isogradvec",
  "isohessian",
  "strength"
};

int
_pullInfoVal[] = {
  pullInfoUnknown,
  pullInfoTensor,             /*  1: [7] tensor here */
  pullInfoTensorInverse,      /*  2: [7] inverse of tensor here */
  pullInfoHessian,            /*  3: [9] hessian used for force distortion */
  pullInfoInside,             /*  4: [1] containment scalar */
  pullInfoInsideGradient,     /*  5: [3] containment vector */
  pullInfoHeight,             /*  6: [1] for gravity */
  pullInfoHeightGradient,     /*  7: [3] for gravity */
  pullInfoHeightHessian,      /*  8: [9] for gravity */
  pullInfoSeedThresh,         /*  9: [1] scalar for thresholding seeding */
  pullInfoTangent1,           /* 10: [3] first tangent to constraint surf */
  pullInfoTangent2,           /* 11: [3] second tangent to constraint surf */
  pullInfoTangentMode,        /* 12: [1] for morphing between co-dim 1 and 2 */
  pullInfoIsosurfaceValue,    /* 13: [1] */
  pullInfoIsosurfaceGradient, /* 14: [3] */
  pullInfoIsosurfaceHessian,  /* 15: [9] */
  pullInfoStrength            /* 16: [1] */
};

airEnum
_pullInfo = {
  "pullInfo",
  PULL_INFO_MAX+1,
  _pullInfoStr, _pullInfoVal,
  NULL,
  NULL, NULL,
  AIR_FALSE
};
airEnum *const
pullInfo = &_pullInfo;

unsigned int
_pullInfoAnswerLen[PULL_INFO_MAX+1] = {
  0, /* pullInfoUnknown */
  7, /* pullInfoTensor */
  7, /* pullInfoTensorInverse */
  9, /* pullInfoHessian */
  1, /* pullInfoInside */
  3, /* pullInfoInsideGradient */
  1, /* pullInfoHeight */
  3, /* pullInfoHeightGradient */
  9, /* pullInfoHeightHessian */
  1, /* pullInfoSeedThresh */
  3, /* pullInfoTangent1 */
  3, /* pullInfoTangent2 */
  1, /* pullInfoTangentMode */
  1, /* pullInfoIsosurfaceValue */
  3, /* pullInfoIsosurfaceGradient */
  9, /* pullInfoIsosurfaceHessian */
  1, /* pullInfoStrength */
}; 

unsigned int
pullInfoAnswerLen(int info) {
  unsigned int ret;
  
  if (!airEnumValCheck(pullInfo, info)) {
    ret = _pullInfoAnswerLen[info];
  } else {
    ret = 0;
  }
  return ret;
}

pullInfoSpec *
pullInfoSpecNew(void) {
  pullInfoSpec *ispec;

  ispec = AIR_CAST(pullInfoSpec *, calloc(1, sizeof(pullInfoSpec)));
  if (ispec) {
    ispec->info = pullInfoUnknown;
    ispec->volName = NULL;
    ispec->item = 0;
    ispec->scale = AIR_NAN;
    ispec->zero = AIR_NAN;
    ispec->constraint = AIR_FALSE;
    ispec->volIdx = UINT_MAX;
  }
  return ispec;
}

pullInfoSpec *
pullInfoSpecNix(pullInfoSpec *ispec) {

  if (ispec) {
    ispec->volName = airFree(ispec->volName);
    airFree(ispec);
  }
  return NULL;
}

int
pullInfoSpecAdd(pullContext *pctx, pullInfoSpec *ispec,
                int info, const char *volName, int item,
                int constraint) {
  char me[]="pullInfoSpecAdd", err[BIFF_STRLEN];
  unsigned int ii, haveLen, needLen;
  const gageKind *kind;
  
  if (!( pctx && ispec && volName )) {
    sprintf(err, "%s: got NULL pointer", me);
    biffAdd(PULL, err); return 1;
  }
  if (airEnumValCheck(pullInfo, info)) {
    sprintf(err, "%s: %d not a valid %s value", me, info, pullInfo->name);
    biffAdd(PULL, err); return 1;
  }
  if (pctx->ispec[info]) {
    sprintf(err, "%s: already set info %s (%d)", me, 
            airEnumStr(pullInfo, info), info);
    biffAdd(PULL, err); return 1;
  }
  for (ii=0; ii<=PULL_INFO_MAX; ii++) {
    if (pctx->ispec[ii] == ispec) {
      sprintf(err, "%s(%s): already got ispec %p as ispec[%u]", me,
              airEnumStr(pullInfo, info), ispec, ii);
      biffAdd(PULL, err); return 1;
    }
  }
  if (0 == pctx->volNum) {
    sprintf(err, "%s(%s): given context has no volumes", me,
            airEnumStr(pullInfo, info));
    biffAdd(PULL, err); return 1;
  }
  for (ii=0; ii<pctx->volNum; ii++) {
    if (!strcmp(pctx->vol[ii]->name, volName)) {
      break;
    }
  }
  if (ii == pctx->volNum) {
    sprintf(err, "%s(%s): no volume has name \"%s\"", me,
            airEnumStr(pullInfo, info), volName);
    biffAdd(PULL, err); return 1;
  }
  kind = pctx->vol[ii]->kind;
  if (airEnumValCheck(kind->enm, item)) {
    sprintf(err, "%s(%s): %d not a valid \"%s\" item", me, 
            airEnumStr(pullInfo, info), item, kind->name);
    biffAdd(PULL, err); return 1;
  }
  needLen = pullInfoAnswerLen(info);
  haveLen = kind->table[item].answerLength;
  if (needLen != haveLen) {
    sprintf(err, "%s(%s): needs len %u, but \"%s\" item \"%s\" has len %u",
            me, airEnumStr(pullInfo, info), needLen,
            kind->name, airEnumStr(kind->enm, item), haveLen);
    biffAdd(PULL, err); return 1;
  }
  if (haveLen > 9) {
    sprintf(err, "%s: sorry, answer length (%u) > 9 unsupported", me,
            haveLen);
    biffAdd(PULL, err); return 1;
  }
  if (constraint && haveLen != 1) {
    sprintf(err, "%s: can't use non-scalar (len %u) %s as constraint", me,
            haveLen, airEnumStr(pullInfo, info));
    biffAdd(PULL, err); return 1;
  }
  /* HEY: this is a needless restriction, isn't it? */
  if (pullInfoStrength == info && !pctx->vol[ii]->ninScale) {
    sprintf(err, "%s(%s): can only use %s info with a stack volume", 
            me, airEnumStr(pullInfo, info), 
            airEnumStr(pullInfo, pullInfoStrength));
    biffAdd(PULL, err); return 1;
  }

  ispec->info = info;
  ispec->volName = airStrdup(volName);
  ispec->volIdx = ii;
  ispec->item = item;
  ispec->constraint = constraint;
  
  /* very tricky: seedOnly is initialized to true for everything, here
     is where we turn it off for anything info that's not seedthresh */
  if (pullInfoSeedThresh != info) {
    pctx->vol[ii]->seedOnly = AIR_FALSE;
  }
  
  /* now set item in gage query */
  gageQueryItemOn(pctx->vol[ii]->gctx, pctx->vol[ii]->gpvl, item);

  pctx->ispec[info] = ispec;
  
  return 0;
}

int
_pullInfoSetup(pullContext *pctx) {
  char me[]="_pullInfoSetup", err[BIFF_STRLEN];
  unsigned int ii;

  pctx->infoTotalLen = 0;
  for (ii=0; ii<=PULL_INFO_MAX; ii++) {
    if (pctx->ispec[ii]) {
      pctx->infoIdx[ii] = pctx->infoTotalLen;
      fprintf(stderr, "!%s: infoIdx[%u] (%s) = %u\n", me,
              ii, airEnumStr(pullInfo, ii), pctx->infoIdx[ii]);
      pctx->infoTotalLen += pullInfoAnswerLen(ii);
      if (!pullInfoAnswerLen(ii)) {
        sprintf(err, "%s: got zero-length answer for ispec[%u]", me, ii);
        biffAdd(PULL, err); return 1;
      }
    }
  }
  fprintf(stderr, "!%s: infoTotalLen = %u\n", me, pctx->infoTotalLen);
  return 0;
}

static void
_infoCopy1(double *dst, const double *src) {
  dst[0] = src[0]; 
}

static void
_infoCopy2(double *dst, const double *src) {
  dst[0] = src[0]; dst[1] = src[1];
}

static void
_infoCopy3(double *dst, const double *src) {
  dst[0] = src[0]; dst[1] = src[1]; dst[2] = src[2];
}

static void
_infoCopy4(double *dst, const double *src) {
  dst[0] = src[0]; dst[1] = src[1]; dst[2] = src[2]; dst[3] = src[3];
}

static void
_infoCopy5(double *dst, const double *src) {
  dst[0] = src[0]; dst[1] = src[1]; dst[2] = src[2]; dst[3] = src[3];
  dst[4] = src[4];
}

static void
_infoCopy6(double *dst, const double *src) {
  dst[0] = src[0]; dst[1] = src[1]; dst[2] = src[2]; dst[3] = src[3];
  dst[4] = src[4]; dst[5] = src[5];
}

static void
_infoCopy7(double *dst, const double *src) {
  dst[0] = src[0]; dst[1] = src[1]; dst[2] = src[2]; dst[3] = src[3];
  dst[4] = src[4]; dst[5] = src[5]; dst[6] = src[6];
}

static void
_infoCopy8(double *dst, const double *src) {
  dst[0] = src[0]; dst[1] = src[1]; dst[2] = src[2]; dst[3] = src[3];
  dst[4] = src[4]; dst[5] = src[5]; dst[6] = src[6]; dst[7] = src[7];
}

static void
_infoCopy9(double *dst, const double *src) {
  dst[0] = src[0]; dst[1] = src[1]; dst[2] = src[2]; dst[3] = src[3];
  dst[4] = src[4]; dst[5] = src[5]; dst[6] = src[6]; dst[7] = src[7];
  dst[8] = src[8];
}

void (*_pullInfoAnswerCopy[10])(double *, const double *) = {
  NULL,
  _infoCopy1,
  _infoCopy2,
  _infoCopy3,
  _infoCopy4,
  _infoCopy5,
  _infoCopy6,
  _infoCopy7,
  _infoCopy8,
  _infoCopy9
};

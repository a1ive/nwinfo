/*
 * Copyright 2008  Veselin Georgiev,
 * anrieffNOSPAM @ mgail_DOT.com (convert to gmail)
 *
 * Redistribution and use in source and binary forms, with or without
 * modification, are permitted provided that the following conditions
 * are met:
 *
 * 1. Redistributions of source code must retain the above copyright
 *    notice, this list of conditions and the following disclaimer.
 * 2. Redistributions in binary form must reproduce the above copyright
 *    notice, this list of conditions and the following disclaimer in the
 *    documentation and/or other materials provided with the distribution.
 *
 * THIS SOFTWARE IS PROVIDED BY THE AUTHOR ``AS IS'' AND ANY EXPRESS OR
 * IMPLIED WARRANTIES, INCLUDING, BUT NOT LIMITED TO, THE IMPLIED WARRANTIES
 * OF MERCHANTABILITY AND FITNESS FOR A PARTICULAR PURPOSE ARE DISCLAIMED.
 * IN NO EVENT SHALL THE AUTHOR BE LIABLE FOR ANY DIRECT, INDIRECT,
 * INCIDENTAL, SPECIAL, EXEMPLARY, OR CONSEQUENTIAL DAMAGES (INCLUDING, BUT
 * NOT LIMITED TO, PROCUREMENT OF SUBSTITUTE GOODS OR SERVICES; LOSS OF USE,
 * DATA, OR PROFITS; OR BUSINESS INTERRUPTION) HOWEVER CAUSED AND ON ANY
 * THEORY OF LIABILITY, WHETHER IN CONTRACT, STRICT LIABILITY, OR TORT
 * (INCLUDING NEGLIGENCE OR OTHERWISE) ARISING IN ANY WAY OUT OF THE USE OF
 * THIS SOFTWARE, EVEN IF ADVISED OF THE POSSIBILITY OF SUCH DAMAGE.
 */
#include <string.h>
#include "libcpuid.h"
#include "libcpuid_ctype.h"
#include "libcpuid_util.h"
#include "libcpuid_internal.h"
#include "recog_intel.h"

const struct intel_bcode_str { intel_code_t code; char *str; } intel_bcode_str[] = {
	#define CODE(x) { x, #x }
	#define CODE2(x, y) CODE(x)
	#include "intel_code_t.h"
	#undef CODE
};

typedef struct {
	int code;
	uint64_t bits;
} intel_code_and_bits_t;

enum _intel_model_t {
	UNKNOWN = -1,
	_3000 = 100,
	_3100,
	_3200,
	X3200,
	_3300,
	X3300,
	_5100,
	_5200,
	_5300,
	_5400,
	_2xxx, /* Core i[357] 2xxx */
	_3xxx, /* Core i[357] 3xxx */
	_4xxx, /* Core i[357] 4xxx */
	_5xxx, /* Core i[357] 5xxx */
	_6xxx, /* Core i[357] 6xxx */
	_7xxx, /* Core i[3579] 7xxx */
	_8xxx, /* Core i[3579] 8xxx */
	_9xxx, /* Core i[3579] 9xxx */
	_10xxx, /* Core i[3579] 10xxx */
	_11xxx, /* Core i[3579] 11xxx */
	_12xxx, /* Core i[3579] 12xxx */
	_13xxx, /* Core i[3579] 13xxx */
	_14xxx, /* Core i[3579] 14xxx */
	_x1xx,  /* Xeon Bronze/Silver/Gold/Platinum x1xx */
	_x2xx,  /* Xeon Bronze/Silver/Gold/Platinum x2xx */
	_x3xx,  /* Xeon Bronze/Silver/Gold/Platinum x3xx */
	_x4xx,  /* Xeon Bronze/Silver/Gold/Platinum/Max x4xx */
	_x5xx,  /* Xeon Bronze/Silver/Gold/Platinum x5xx */
	_1xx,   /* Core Ultra [3579] 1xx */
	_2xx,   /* Core Ultra [3579] 2xx */
};
typedef enum _intel_model_t intel_model_t;

const struct match_entry_t cpudb_intel[] = {
//     F   M   S  EF  EM  #cores  L2$   L3$  BC  ModelBits     ModelCode  Name
	{ -1, -1, -1, -1, -1,   1,    -1,    -1, NC, 0             ,     0, "Unknown Intel CPU"       },

	/* i486 */
	{  4, -1, -1, -1, -1,   1,    -1,    -1, NC, 0             ,     0, "Unknown i486"            },
	{  4,  0, -1, -1, -1,   1,    -1,    -1, NC, 0             ,     0, "i486 DX-25/33"           },
	{  4,  1, -1, -1, -1,   1,    -1,    -1, NC, 0             ,     0, "i486 DX-50"              },
	{  4,  2, -1, -1, -1,   1,    -1,    -1, NC, 0             ,     0, "i486 SX"                 },
	{  4,  3, -1, -1, -1,   1,    -1,    -1, NC, 0             ,     0, "i486 DX2"                },
	{  4,  4, -1, -1, -1,   1,    -1,    -1, NC, 0             ,     0, "i486 SL"                 },
	{  4,  5, -1, -1, -1,   1,    -1,    -1, NC, 0             ,     0, "i486 SX2"                },
	{  4,  7, -1, -1, -1,   1,    -1,    -1, NC, 0             ,     0, "i486 DX2 WriteBack"      },
	{  4,  8, -1, -1, -1,   1,    -1,    -1, NC, 0             ,     0, "i486 DX4"                },
	{  4,  9, -1, -1, -1,   1,    -1,    -1, NC, 0             ,     0, "i486 DX4 WriteBack"      },

	/* All Pentia:
	   Pentium 1 */
	{  5, -1, -1, -1, -1,   1,    -1,    -1, NC, 0             ,     0, "Unknown Pentium"         },
	{  5,  0, -1, -1, -1,   1,    -1,    -1, NC, 0             ,     0, "Pentium A-Step"          },
	{  5,  1, -1, -1, -1,   1,    -1,    -1, NC, 0             ,     0, "Pentium 1 (0.8u)"        },
	{  5,  2, -1, -1, -1,   1,    -1,    -1, NC, 0             ,     0, "Pentium 1 (0.35u)"       },
	{  5,  3, -1, -1, -1,   1,    -1,    -1, NC, 0             ,     0, "Pentium OverDrive"       },
	{  5,  4, -1, -1, -1,   1,    -1,    -1, NC, 0             ,     0, "Pentium 1 (0.35u)"       },
	{  5,  7, -1, -1, -1,   1,    -1,    -1, NC, 0             ,     0, "Pentium 1 (0.35u)"       },
	{  5,  8, -1, -1, -1,   1,    -1,    -1, NC, 0             ,     0, "Pentium MMX (0.25u)"     },

	/* Pentium 2 / 3 / M / Conroe / whatsnext - all P6 based. */
	{  6, -1, -1, -1, -1,   1,    -1,    -1, NC, 0             ,     0, "Unknown P6"              },
	{  6,  0, -1, -1, -1,   1,    -1,    -1, NC, 0             ,     0, "Pentium Pro"             },
	{  6,  1, -1, -1, -1,   1,    -1,    -1, NC, 0             ,     0, "Pentium Pro"             },
	{  6,  3, -1, -1, -1,   1,    -1,    -1, NC, 0             ,     0, "Pentium II (Klamath)"    },
	{  6,  5, -1, -1, -1,   1,    -1,    -1, NC, 0             ,     0, "Pentium II (Deschutes)"  },
	{  6,  5, -1, -1, -1,   1,    -1,    -1, NC, MOBILE_|PENTIUM_,   0, "Mobile Pentium II (Tonga)"},
	{  6,  6, -1, -1, -1,   1,    -1,    -1, NC,0              ,     0, "Pentium II (Dixon)"      },

	{  6,  3, -1, -1, -1,   1,    -1,    -1, NC, XEON_         ,     0, "P-II Xeon (Klamath)"     },
	{  6,  5, -1, -1, -1,   1,    -1,    -1, NC, XEON_         ,     0, "P-II Xeon (Drake)"       },
	{  6,  6, -1, -1, -1,   1,    -1,    -1, NC, XEON_         ,     0, "P-II Xeon (Dixon)"       },

	{  6,  5, -1, -1, -1,   1,    -1,    -1, NC, CELERON_      ,     0, "P-II Celeron (Covington)" },
	{  6,  6, -1, -1, -1,   1,    -1,    -1, NC, CELERON_      ,     0, "P-II Celeron (Mendocino)" },

	/* -------------------------------------------------- */

	{  6,  7, -1, -1, -1,   1,    -1,    -1, NC, 0             ,     0, "Pentium III (Katmai)"    },
	{  6,  8, -1, -1, -1,   1,    -1,    -1, NC, 0             ,     0, "Pentium III (Coppermine)"},
	{  6, 10, -1, -1, -1,   1,    -1,    -1, NC, 0             ,     0, "Pentium III (Coppermine)"},
	{  6, 11, -1, -1, -1,   1,    -1,    -1, NC, 0             ,     0, "Pentium III (Tualatin)"  },
	{  6, 11, -1, -1, -1,   1,   512,    -1, NC, 0             ,     0, "Pentium III (Tualatin)"  },

	{  6,  7, -1, -1, -1,   1,    -1,    -1, NC, XEON_         ,     0, "P-III Xeon (Tanner)"     },
	{  6,  8, -1, -1, -1,   1,    -1,    -1, NC, XEON_         ,     0, "P-III Xeon (Cascades)"   },
	{  6, 10, -1, -1, -1,   1,    -1,    -1, NC, XEON_         ,     0, "P-III Xeon (Cascades)"   },
	{  6, 11, -1, -1, -1,   1,    -1,    -1, NC, XEON_         ,     0, "P-III Xeon (Tualatin)"   },

	{  6,  7, -1, -1, -1,   1,   128,    -1, NC, CELERON_      ,     0, "P-III Celeron (Katmai)"     },
	{  6,  8, -1, -1, -1,   1,   128,    -1, NC, CELERON_      ,     0, "P-III Celeron (Coppermine)" },
	{  6, 10, -1, -1, -1,   1,   128,    -1, NC, CELERON_      ,     0, "P-III Celeron (Coppermine)" },
	{  6, 11, -1, -1, -1,   1,   256,    -1, NC, CELERON_      ,     0, "P-III Celeron (Tualatin)"   },

	/* Netburst based (Pentium 4 and later)
	   classic P4s */
	{ 15, -1, -1, -1, -1,   1,    -1,    -1, NC, 0             ,     0, "Unknown Pentium 4"       },
	{ 15, -1, -1, 15, -1,   1,    -1,    -1, NC, CELERON_      ,     0, "Unknown P-4 Celeron"     },
	{ 15, -1, -1, 15, -1,   1,    -1,    -1, NC, XEON_         ,     0, "Unknown Xeon"            },

	{ 15,  0, -1, 15, -1,   1,    -1,    -1, NC, PENTIUM_      ,     0, "Pentium 4 (Willamette)"  },
	{ 15,  1, -1, 15, -1,   1,    -1,    -1, NC, PENTIUM_      ,     0, "Pentium 4 (Willamette)"  },
	{ 15,  2, -1, 15, -1,   1,    -1,    -1, NC, PENTIUM_      ,     0, "Pentium 4 (Northwood)"   },
	{ 15,  3, -1, 15, -1,   1,    -1,    -1, NC, PENTIUM_      ,     0, "Pentium 4 (Prescott)"    },
	{ 15,  4, -1, 15, -1,   1,    -1,    -1, NC, PENTIUM_      ,     0, "Pentium 4 (Prescott)"    },
	{ 15,  6, -1, 15, -1,   1,    -1,    -1, NC, PENTIUM_      ,     0, "Pentium 4 (Cedar Mill)"  },
	{ 15,  0, -1, 15, -1,   1,    -1,    -1, NC, MOBILE_|PENTIUM_,   0, "Mobile P-4 (Willamette)" },
	{ 15,  1, -1, 15, -1,   1,    -1,    -1, NC, MOBILE_|PENTIUM_,   0, "Mobile P-4 (Willamette)" },
	{ 15,  2, -1, 15, -1,   1,    -1,    -1, NC, MOBILE_|PENTIUM_,   0, "Mobile P-4 (Northwood)"  },
	{ 15,  3, -1, 15, -1,   1,    -1,    -1, NC, MOBILE_|PENTIUM_,   0, "Mobile P-4 (Prescott)"   },
	{ 15,  4, -1, 15, -1,   1,    -1,    -1, NC, MOBILE_|PENTIUM_,   0, "Mobile P-4 (Prescott)"   },
	{ 15,  6, -1, 15, -1,   1,    -1,    -1, NC, MOBILE_|PENTIUM_,   0, "Mobile P-4 (Cedar Mill)" },

	/* server CPUs */
	{ 15,  0, -1, 15, -1,   1,    -1,    -1, NC, XEON_         ,     0, "Xeon (Foster)"           },
	{ 15,  1, -1, 15, -1,   1,    -1,    -1, NC, XEON_         ,     0, "Xeon (Foster)"           },
	{ 15,  2, -1, 15, -1,   1,    -1,    -1, NC, XEON_         ,     0, "Xeon (Prestonia)"        },
	{ 15,  2, -1, 15, -1,   1,    -1,    -1, NC, XEON_|_MP_    ,     0, "Xeon (Gallatin)"         },
	{ 15,  3, -1, 15, -1,   1,    -1,    -1, NC, XEON_         ,     0, "Xeon (Nocona)"           },
	{ 15,  4, -1, 15, -1,   1,    -1,    -1, NC, XEON_         ,     0, "Xeon (Nocona)"           },
	{ 15,  4, -1, 15, -1,   1,    -1,    -1, IRWIN, XEON_      ,     0, "Xeon (Irwindale)"        },
	{ 15,  4, -1, 15, -1,   1,    -1,    -1, NC, XEON_|_MP_    ,     0, "Xeon (Cranford)"         },
	{ 15,  4, -1, 15, -1,   1,    -1,    -1, POTOMAC, XEON_    ,     0, "Xeon (Potomac)"          },
	{ 15,  6, -1, 15, -1,   1,    -1,    -1, NC, XEON_         ,     0, "Xeon (Dempsey)"          },

	/* Pentium Ds */
	{ 15,  4,  4, 15, -1,   1,    -1,    -1, NC, 0             ,     0, "Pentium D (SmithField)"  },
	{ 15,  4, -1, 15, -1,   1,    -1,    -1, PENTIUM_D, 0      ,     0, "Pentium D (SmithField)"  },
	{ 15,  4,  7, 15, -1,   1,    -1,    -1, NC, 0             ,     0, "Pentium D (SmithField)"  },
	{ 15,  6, -1, 15, -1,   1,    -1,    -1, PENTIUM_D, 0      ,     0, "Pentium D (Presler)"     },

	/* Celeron and Celeron Ds */
	{ 15,  1, -1, 15, -1,   1,    -1,    -1, NC, CELERON_      ,     0, "P-4 Celeron (Willamette)"   },
	{ 15,  2, -1, 15, -1,   1,    -1,    -1, NC, CELERON_      ,     0, "P-4 Celeron (Northwood)"    },
	{ 15,  3, -1, 15, -1,   1,    -1,    -1, NC, CELERON_      ,     0, "P-4 Celeron D (Prescott)"   },
	{ 15,  4, -1, 15, -1,   1,    -1,    -1, NC, CELERON_      ,     0, "P-4 Celeron D (Prescott)"   },
	{ 15,  6, -1, 15, -1,   1,    -1,    -1, NC, CELERON_      ,     0, "P-4 Celeron D (Cedar Mill)" },

	/* -------------------------------------------------- */
	/* Intel Core microarchitecture - P6-based */

	{  6,  9, -1, -1, -1,   1,    -1,    -1, NC, 0             ,     0, "Unknown Pentium M"          },
	{  6,  9, -1, -1, -1,   1,    -1,    -1, PENTIUM_M, 0      ,     0, "Unknown Pentium M"          },
	{  6,  9, -1, -1, -1,   1,    -1,    -1, NC, PENTIUM_      ,     0, "Pentium M (Banias)"         },
	{  6,  9, -1, -1, -1,   1,    -1,    -1, PENTIUM_M, 0      ,     0, "Pentium M (Banias)"         },
	{  6,  9, -1, -1, -1,   1,    -1,    -1, NC, CELERON_      ,     0, "Celeron M"                  },
	{  6, 13, -1, -1, -1,   1,    -1,    -1, NC, PENTIUM_      ,     0, "Pentium M (Dothan)"         },
	{  6, 13, -1, -1, -1,   1,    -1,    -1, PENTIUM_M, 0      ,     0, "Pentium M (Dothan)"         },
	{  6, 13, -1, -1, -1,   1,    -1,    -1, NC, CELERON_      ,     0, "Celeron M"                  },

	{  6, 12, -1, -1, -1,  -1,    -1,    -1, NC, ATOM_         ,     0, "Unknown Atom"               },
	{  6, 12, -1, -1, -1,  -1,    -1,    -1, DIAMONDVILLE,ATOM_,     0, "Atom (Diamondville)"        },
	{  6, 12, -1, -1, -1,  -1,    -1,    -1, SILVERTHORNE,ATOM_,     0, "Atom (Silverthorne)"        },
	{  6, 12, -1, -1, -1,  -1,    -1,    -1, CEDARVIEW, ATOM_  ,     0, "Atom (Cedarview)"           },
	{  6,  6, -1, -1, -1,  -1,    -1,    -1, CEDARVIEW, ATOM_  ,     0, "Atom (Cedarview)"           },
	{  6, 12, -1, -1, -1,  -1,    -1,    -1, PINEVIEW, ATOM_   ,     0, "Atom (Pineview)"            },

	/* -------------------------------------------------- */

	{  6, 14, -1, -1, -1,   1,    -1,    -1, NC, 0             ,     0, "Unknown Yonah"             },
	{  6, 14, -1, -1, -1,   1,    -1,    -1, CORE_SOLO, 0      ,     0, "Yonah (Core Solo)"         },
	{  6, 14, -1, -1, -1,   2,    -1,    -1, CORE_DUO, 0       ,     0, "Yonah (Core Duo)"          },
	{  6, 14, -1, -1, -1,   1,    -1,    -1, CORE_SOLO, MOBILE_,     0, "Yonah (Core Solo)"         },
	{  6, 14, -1, -1, -1,   2,    -1,    -1, CORE_DUO , MOBILE_,     0, "Yonah (Core Duo)"          },
	{  6, 14, -1, -1, -1,   1,    -1,    -1, CORE_SOLO, 0      ,     0, "Yonah (Core Solo)"         },

	{  6, 15, -1, -1, -1,   1,    -1,    -1, NC, 0             ,     0, "Unknown Core 2"            },
	{  6, 15, -1, -1, -1,   2,  4096,    -1, CORE_DUO, 0       ,     0, "Conroe (Core 2 Duo)"       },
	{  6, 15, -1, -1, -1,   2,  1024,    -1, CORE_DUO, 0       ,     0, "Conroe (Core 2 Duo) 1024K" },
	{  6, 15, -1, -1, -1,   2,   512,    -1, CORE_DUO, 0       ,     0, "Conroe (Core 2 Duo) 512K"  },
	{  6, 15, -1, -1, -1,   4,    -1,    -1, QUAD_CORE, 0      ,     0, "Kentsfield (Core 2 Quad)"  },
	{  6, 15, -1, -1, -1,   4,  4096,    -1, QUAD_CORE, 0      ,     0, "Kentsfield (Core 2 Quad)"  },
	{  6, 15, -1, -1, -1, 400,    -1,    -1, MORE_THAN_QUADCORE, 0,  0, "More than quad-core"       },
	{  6, 15, -1, -1, -1,   2,  2048,    -1, CORE_DUO, 0       ,     0, "Allendale (Core 2 Duo)"    },
	{  6, 15, -1, -1, -1,   2,    -1,    -1, MOBILE_CORE_DUO, 0,     0, "Merom (Core 2 Duo)"        },
	{  6, 15, -1, -1, -1,   2,  2048,    -1, MEROM, 0          ,     0, "Merom (Core 2 Duo) 2048K"  },
	{  6, 15, -1, -1, -1,   2,  4096,    -1, MEROM, 0          ,     0, "Merom (Core 2 Duo) 4096K"  },

	{  6, 15, -1, -1, 15,   2,    -1,    -1, NC, PENTIUM_      ,     0, "Allendale (Pentium)"       },
	{  6, 15, -1, -1, 15,   2,    -1,    -1, NC, CELERON_      ,     0, "Allendale (Celeron)"      },
	{  6,  6, -1, -1, 22,   1,    -1,    -1, NC, CELERON_      ,     0, "Conroe-L (Celeron)"        },


	{  6,  6, -1, -1, 22,   1,    -1,    -1, NC, 0             ,     0, "Unknown Core ?"           },
	{  6,  7, -1, -1, 23,   1,    -1,    -1, NC, 0             ,     0, "Unknown Core ?"           },
	{  6,  6, -1, -1, 22, 400,    -1,    -1, MORE_THAN_QUADCORE, 0,  0, "More than quad-core"      },
	{  6,  7, -1, -1, 23, 400,    -1,    -1, MORE_THAN_QUADCORE, 0,  0, "More than quad-core"      },

	{  6,  7, -1, -1, 23,   1,    -1,    -1, CORE_SOLO         , 0,  0, "Unknown Core 45nm"        },
	{  6,  7, -1, -1, 23,   1,    -1,    -1, CORE_DUO          , 0,  0, "Unknown Core 45nm"        },
	{  6,  7, -1, -1, 23,   2,  1024,    -1, WOLFDALE          , 0,  0, "Celeron Wolfdale 1M"      },
	{  6,  7, -1, -1, 23,   2,  2048,    -1, WOLFDALE          , 0,  0, "Wolfdale (Core 2 Duo) 2M" },
	{  6,  7, -1, -1, 23,   2,  3072,    -1, WOLFDALE          , 0,  0, "Wolfdale (Core 2 Duo) 3M" },
	{  6,  7, -1, -1, 23,   2,  6144,    -1, WOLFDALE          , 0,  0, "Wolfdale (Core 2 Duo) 6M" },
	{  6,  7, -1, -1, 23,   1,  1024,    -1, PENRYN,      CELERON_,  0, "Celeron Penryn L"         },
	{  6,  7, -1, -1, 23,   1,    -1,    -1, MOBILE_CORE_DUO   , 0,  0, "Penryn (Core 2 Duo)"      },
	{  6,  7, -1, -1, 23,   2,  1024,    -1, PENRYN            , 0,  0, "Penryn (Core 2 Duo)"      },
	{  6,  7, -1, -1, 23,   2,  3072,    -1, PENRYN            , 0,  0, "Penryn (Core 2 Duo) 3M"   },
	{  6,  7, -1, -1, 23,   2,  6144,    -1, PENRYN            , 0,  0, "Penryn (Core 2 Duo) 6M"   },
	{  6,  7, -1, -1, 23,   4,  2048,    -1, NC                , 0,  0, "Yorkfield (Core 2 Quad) 2M"},
	{  6,  7, -1, -1, 23,   4,  3072,    -1, NC                , 0,  0, "Yorkfield (Core 2 Quad) 3M"},
	{  6,  7, -1, -1, 23,   4,  6144,    -1, NC                , 0,  0, "Yorkfield (Core 2 Quad) 6M"},

	/* Core microarchitecture-based Xeons: */
	{  6, 14, -1, -1, 14,   1,    -1,    -1, NC, XEON_         ,     0, "Xeon LV"                  },
	{  6, 15, -1, -1, 15,   2,  4096,    -1, NC, XEON_         , _5100, "Xeon (Woodcrest)"         },
	{  6, 15, -1, -1, 15,   2,  2048,    -1, NC, XEON_         , _3000, "Xeon (Conroe/2M)"         },
	{  6, 15, -1, -1, 15,   2,  4096,    -1, NC, XEON_         , _3000, "Xeon (Conroe/4M)"         },
	{  6, 15, -1, -1, 15,   4,  4096,    -1, NC, XEON_         , X3200, "Xeon (Kentsfield)"        },
	{  6, 15, -1, -1, 15,   4,  4096,    -1, NC, XEON_         , _5300, "Xeon (Clovertown)"        },
	{  6,  7, -1, -1, 23,   2,  6144,    -1, NC, XEON_         , _3100, "Xeon (Wolfdale)"          },
	{  6,  7, -1, -1, 23,   2,  6144,    -1, NC, XEON_         , _5200, "Xeon (Wolfdale DP)"       },
	{  6,  7, -1, -1, 23,   4,  6144,    -1, NC, XEON_         , _5400, "Xeon (Harpertown)"        },
	{  6,  7, -1, -1, 23,   4,  3072,    -1, NC, XEON_         , X3300, "Xeon (Yorkfield/3M)"      },
	{  6,  7, -1, -1, 23,   4,  6144,    -1, NC, XEON_         , X3300, "Xeon (Yorkfield/6M)"      },

	/* Nehalem CPUs (45nm): */
	{  6, 10, -1, -1, 26,   4,    -1,    -1, GAINESTOWN, XEON_ ,     0, "Gainestown (Xeon)"        },
	{  6, 10, -1, -1, 26,   4,    -1,  4096, GAINESTOWN, XEON_ ,     0, "Gainestown 4M (Xeon)"     },
	{  6, 10, -1, -1, 26,   4,    -1,  8192, GAINESTOWN, XEON_ ,     0, "Gainestown 8M (Xeon)"     },
	{  6, 10, -1, -1, 26,   4,    -1,    -1, NC, XEON_|_7      ,     0, "Bloomfield (Xeon)"        },
	{  6, 10, -1, -1, 26,   4,    -1,    -1, NC, CORE_|_I_|_7  ,     0, "Bloomfield (Core i7)"     },
	{  6, 10, -1, -1, 30,   4,    -1,    -1, NC, CORE_|_I_|_7  ,     0, "Lynnfield (Core i7)"      },
	{  6,  5, -1, -1, 30,   4,    -1,  8192, NC, CORE_|_I_|_5  ,     0, "Lynnfield (Core i5)"      },

	/* Westmere CPUs (32nm): */
	{  6,  5, -1, -1, 37,   2,    -1,    -1, NC, 0             ,     0, "Unknown Core i3/i5"       },
	{  6, 12, -1, -1, 44,  -1,    -1,    -1, WESTMERE, XEON_   ,     0, "Westmere (Xeon)"          },
	{  6, 12, -1, -1, 44,  -1,    -1, 12288, WESTMERE, XEON_   ,     0, "Gulftown (Xeon)"          },
	{  6, 12, -1, -1, 44,   4,    -1, 12288, NC, CORE_|_I_|_7  ,     0, "Gulftown (Core i7)"       },
	{  6,  5, -1, -1, 37,   2,    -1,  4096, NC, CORE_|_I_|_5  ,     0, "Clarkdale (Core i5)"      },
	{  6,  5, -1, -1, 37,   2,    -1,  4096, NC, CORE_|_I_|_3  ,     0, "Clarkdale (Core i3)"      },
	{  6,  5, -1, -1, 37,   2,    -1,  4096, NC, CORE_|_I_|_7  ,     0, "Arrandale (Core i7)"      },
	{  6,  5, -1, -1, 37,   2,    -1,  3072, NC, CORE_|_I_|_5  ,     0, "Arrandale (Core i5)"      },
	{  6,  5, -1, -1, 37,   2,    -1,  3072, NC, CORE_|_I_|_3  ,     0, "Arrandale (Core i3)"      },
	{  6,  5, -1, -1, 37,   2,    -1,    -1, NC, PENTIUM_      ,     0, "Arrandale (Pentium)"      },
	{  6,  5, -1, -1, 37,   2,    -1,    -1, NC, CELERON_      ,     0, "Arrandale (Celeron)"      },

	/* Sandy Bridge CPUs (2nd gen, 32nm): */
	{  6, 10, -1, -1, 42,  -1,    -1,    -1, NC, 0             ,     0, "Unknown Sandy Bridge"     },
	{  6, 10, -1, -1, 42,  -1,    -1,    -1, NC, XEON_         ,     0, "Sandy Bridge (Xeon)"      },
	{  6, 10, -1, -1, 42,   4,    -1,    -1, NC, XEON_         ,     0, "Sandy Bridge (Xeon)"      },
	{  6, 10, -1, -1, 42,  -1,    -1,    -1, NC, CORE_|_I_|_7  ,     0, "Sandy Bridge (Core i7)"   },
	{  6, 10, -1, -1, 42,   4,    -1,    -1, NC, CORE_|_I_|_7  ,     0, "Sandy Bridge (Core i7)"   },
	{  6, 10, -1, -1, 42,   4,    -1,    -1, NC, CORE_|_I_|_5  ,     0, "Sandy Bridge (Core i5)"   },
	{  6, 10, -1, -1, 42,   2,    -1,    -1, NC, CORE_|_I_|_3  ,     0, "Sandy Bridge (Core i3)"   },
	{  6, 10, -1, -1, 42,   2,    -1,    -1, NC, PENTIUM_      ,     0, "Sandy Bridge (Pentium)"   },
	{  6, 10, -1, -1, 42,   1,    -1,    -1, NC, CELERON_      ,     0, "Sandy Bridge (Celeron)"   },
	{  6, 10, -1, -1, 42,   2,    -1,    -1, NC, CELERON_      ,     0, "Sandy Bridge (Celeron)"   },
	{  6, 13, -1, -1, 45,  -1,    -1,    -1, NC, CORE_|_I_|_7  ,     0, "Sandy Bridge-E (Core i7)" },
	{  6, 13, -1, -1, 45,  -1,    -1,    -1, NC, XEON_         ,     0, "Sandy Bridge-E (Xeon)"    },

	/* Ivy Bridge CPUs (3rd gen, 22nm): */
	{  6, 10, -1, -1, 58,  -1,    -1,    -1, NC, XEON_         ,     0, "Ivy Bridge (Xeon)"        },
	{  6, 10, -1, -1, 58,   4,    -1,    -1, NC, CORE_|_I_|_7  ,     0, "Ivy Bridge (Core i7)"     },
	{  6, 10, -1, -1, 58,   4,    -1,    -1, NC, CORE_|_I_|_5  ,     0, "Ivy Bridge (Core i5)"     },
	{  6, 10, -1, -1, 58,   2,    -1,    -1, NC, CORE_|_I_|_3  ,     0, "Ivy Bridge (Core i3)"     },
	{  6, 10, -1, -1, 58,   2,    -1,    -1, NC, PENTIUM_      ,     0, "Ivy Bridge (Pentium)"     },
	{  6, 10, -1, -1, 58,   1,    -1,    -1, NC, CELERON_      ,     0, "Ivy Bridge (Celeron)"     },
	{  6, 10, -1, -1, 58,   2,    -1,    -1, NC, CELERON_      ,     0, "Ivy Bridge (Celeron)"     },
	{  6, 14, -1, -1, 62,  -1,    -1,    -1, NC, XEON_         ,     0, "Ivy Bridge-E (Xeon)"      },
	{  6, 14, -1, -1, 62,  -1,    -1,    -1, NC, 0             ,     0, "Ivy Bridge-E"             },

	/* Haswell CPUs (4th gen, 22nm): */
	{  6, 12, -1, -1, 60,  -1,    -1,    -1, NC, XEON_         ,     0, "Haswell (Xeon)"           },
	{  6, 12, -1, -1, 60,   4,    -1,    -1, NC, CORE_|_I_|_7  ,     0, "Haswell (Core i7)"        },
	{  6,  5, -1, -1, 69,   4,    -1,    -1, NC, CORE_|_I_|_7  ,     0, "Haswell (Core i7)"        },
	{  6,  6, -1, -1, 70,   4,    -1,    -1, NC, CORE_|_I_|_7  ,     0, "Haswell (Core i7)"        },
	{  6, 12, -1, -1, 60,   4,    -1,    -1, NC, CORE_|_I_|_5  ,     0, "Haswell (Core i5)"        },
	{  6,  5, -1, -1, 69,   4,    -1,    -1, NC, CORE_|_I_|_5  ,     0, "Haswell (Core i5)"        },
	{  6, 12, -1, -1, 60,   2,    -1,    -1, NC, CORE_|_I_|_5  ,     0, "Haswell (Core i5)"        },
	{  6,  5, -1, -1, 69,   2,    -1,    -1, NC, CORE_|_I_|_5  ,     0, "Haswell (Core i5)"        },
	{  6, 12, -1, -1, 60,   2,    -1,    -1, NC, CORE_|_I_|_3  ,     0, "Haswell (Core i3)"        },
	{  6,  5, -1, -1, 69,   2,    -1,    -1, NC, CORE_|_I_|_3  ,     0, "Haswell (Core i3)"        },
	{  6, 12, -1, -1, 60,   2,    -1,    -1, NC, PENTIUM_      ,     0, "Haswell (Pentium)"        },
	{  6, 12, -1, -1, 60,   2,    -1,    -1, NC, CELERON_      ,     0, "Haswell (Celeron)"        },
	{  6, 12, -1, -1, 60,   1,    -1,    -1, NC, CELERON_      ,     0, "Haswell (Celeron)"        },
	{  6, 15, -1, -1, 63,  -1,    -1,    -1, NC, 0             ,     0, "Haswell-E"                },

	/* Silvermont CPUs (2013, 22nm, low-power) */
	{  6,  7, -1, -1, 55,  -1,    -1,    -1, NC, PENTIUM_|_J_  ,     0, "Bay Trail-D (Pentium)"    },
	{  6,  7, -1, -1, 55,  -1,    -1,    -1, NC, CELERON_|_J_  ,     0, "Bay Trail-D (Celeron)"    },
	{  6,  7, -1, -1, 55,  -1,    -1,    -1, NC, PENTIUM_|_N_  ,     0, "Bay Trail-M (Pentium)"    },
	{  6,  7, -1, -1, 55,  -1,    -1,    -1, NC, CELERON_|_N_  ,     0, "Bay Trail-M (Celeron)"    },
	{  6,  7, -1, -1, 55,  -1,    -1,    -1, NC, ATOM_         ,     0, "Bay Trail-T (Atom)"       },

	/* Broadwell CPUs (5th gen, 14nm): */
	{  6,  7, -1, -1, 71,   4,    -1,    -1, NC, CORE_|_I_|_7  ,     0, "Broadwell (Core i7)"      },
	{  6,  7, -1, -1, 71,   4,    -1,    -1, NC, CORE_|_I_|_5  ,     0, "Broadwell (Core i5)"      },
	{  6, 13, -1, -1, 61,   4,    -1,    -1, NC, CORE_|_I_|_7  ,     0, "Broadwell-U (Core i7)"    },
	{  6, 13, -1, -1, 61,   2,    -1,    -1, NC, CORE_|_I_|_7  ,     0, "Broadwell-U (Core i7)"    },
	{  6, 13, -1, -1, 61,   2,    -1,    -1, NC, CORE_|_I_|_5  ,     0, "Broadwell-U (Core i5)"    },
	{  6, 13, -1, -1, 61,   2,    -1,    -1, NC, CORE_|_I_|_3  ,     0, "Broadwell-U (Core i3)"    },
	{  6, 13, -1, -1, 61,   2,    -1,    -1, NC, PENTIUM_      ,     0, "Broadwell-U (Pentium)"    },
	{  6, 13, -1, -1, 61,   2,    -1,    -1, NC, CELERON_      ,     0, "Broadwell-U (Celeron)"    },
	{  6, 13, -1, -1, 61,   2,    -1,    -1, NA, 0             ,     0, "Broadwell-U (Core M)"     },
	{  6, 15, -1, -1, 79,  -1,    -1,    -1, NC, XEON_         ,     0, "Broadwell-E (Xeon)"       },
	{  6, 15, -1, -1, 79,   2,    -1,    -1, NC, CORE_|_I_|_3  ,     0, "Broadwell-E (Core i3)"    },
	{  6, 15, -1, -1, 79,   2,    -1,    -1, NC, CORE_|_I_|_5  ,     0, "Broadwell-E (Core i5)"    },
	{  6, 15, -1, -1, 79,   4,    -1,    -1, NC, CORE_|_I_|_5  ,     0, "Broadwell-E (Core i5)"    },
	{  6, 15, -1, -1, 79,   2,    -1,    -1, NC, CORE_|_I_|_7  ,     0, "Broadwell-E (Core i7)"    },
	{  6, 15, -1, -1, 79,   4,    -1,    -1, NC, CORE_|_I_|_7  ,     0, "Broadwell-E (Core i7)"    },

	/* Skylake (client) CPUs (2015, 6th Core i gen, 14nm) => https://en.wikichip.org/wiki/intel/microarchitectures/skylake_(client) */
	{  6, 14, -1, -1, 94,  -1,    -1,    -1, NC, XEON_            ,     0, "Skylake (Xeon)"             },
	{  6, 14, -1, -1, 94,   4,    -1,    -1, NC, CORE_|_I_|_7     , _6xxx, "Skylake (Core i7)"          },
	{  6, 14, -1, -1, 94,   4,    -1,    -1, NC, CORE_|_I_|_5     , _6xxx, "Skylake (Core i5)"          },
	{  6, 14, -1, -1, 94,   2,    -1,    -1, NC, CORE_|_I_|_3     , _6xxx, "Skylake (Core i3)"          },
	{  6, 14, -1, -1, 94,   2,    -1,    -1, NC, PENTIUM_         ,     0, "Skylake (Pentium)"          },
	{  6, 14, -1, -1, 78,   2,    -1,    -1, NC, PENTIUM_         ,     0, "Skylake (Pentium)"          },
	{  6, 14, -1, -1, 94,   2,    -1,    -1, NC, CELERON_         ,     0, "Skylake (Celeron)"          },
	{  6, 14, -1, -1, 78,   2,    -1,    -1, NC, CELERON_         ,     0, "Skylake (Celeron)"          },
	{  6, 14, -1, -1, 78,   2,    -1,    -1, NC, CORE_|_M_|_7     , _6xxx, "Skylake (Core m7)"          },
	{  6, 14, -1, -1, 78,   2,    -1,    -1, NC, CORE_|_M_|_5     , _6xxx, "Skylake (Core m5)"          },
	{  6, 14, -1, -1, 78,   2,    -1,    -1, NC, CORE_|_M_|_3     , _6xxx, "Skylake (Core m3)"          },
	/* Skylake (server) CPUs (2017, 1st Xeon Scalable gen, 14nm) => https://en.wikichip.org/wiki/intel/microarchitectures/skylake_(server) */
	{  6,  5, -1, -1, 85,  -1,    -1,    -1, NC, CORE_|_I_|_9     , _6xxx, "Skylake-X (Core i9)"        }, /* 10 to 18 cores */
	{  6,  5, -1, -1, 85,  -1,    -1,    -1, NC, CORE_|_I_|_7     , _6xxx, "Skylake-X (Core i7)"        }, /* 6 to 8 cores */
	{  6,  5, -1, -1, 85,  -1,    -1,    -1, NC, XEON_|_W_        , _x1xx, "Skylake-W (Xeon W)"         },
	{  6,  5, -1, -1, 85,  -1,    -1,    -1, NC, XEON_|_D_        , _x1xx, "Skylake-DE (Xeon D)"        },
	{  6,  5, -1, -1, 85,  -1,    -1,    -1, NC, XEON_|_PLATINIUM_, _x1xx, "Skylake-SP (Xeon Platinum)" },
	{  6,  5, -1, -1, 85,  -1,    -1,    -1, NC, XEON_|_GOLD_     , _x1xx, "Skylake-SP (Xeon Gold)"     },
	{  6,  5, -1, -1, 85,  -1,    -1,    -1, NC, XEON_|_SILVER_   , _x1xx, "Skylake-SP (Xeon Silver)"   },
	{  6,  5, -1, -1, 85,  -1,    -1,    -1, NC, XEON_|_BRONZE_   , _x1xx, "Skylake-SP (Xeon Bronze)"   },

	/* Kaby Lake CPUs (7th gen, 14nm): */
	{  6, 14, -1, -1, 158,  4,    -1,    -1, NC, CORE_|_I_|_7  , _7xxx, "Kaby Lake (Core i7)"      },
	{  6, 14, -1, -1, 158,  4,    -1,    -1, NC, CORE_|_I_|_5  , _7xxx, "Kaby Lake (Core i5)"      },
	{  6, 14, -1, -1, 158,  2,    -1,    -1, NC, CORE_|_I_|_3  , _7xxx, "Kaby Lake (Core i3)"      },
	{  6, 14, -1, -1, 158,  2,    -1,    -1, NC, PENTIUM_      ,     0, "Kaby Lake (Pentium)"      },
	{  6, 14, -1, -1, 158,  2,    -1,    -1, NC, CELERON_      ,     0, "Kaby Lake (Celeron)"      },
	{  6, 14,  9, -1, 142,  2,    -1,    -1, NC, CORE_|_I_|_7  , _7xxx, "Kaby Lake-U (Core i7)"    },
	{  6, 14,  9, -1, 142,  2,    -1,    -1, NC, CORE_|_I_|_5  , _7xxx, "Kaby Lake-U (Core i5)"    },
	{  6, 14,  9, -1, 142,  2,    -1,    -1, NC, CORE_|_I_|_3  , _7xxx, "Kaby Lake-U (Core i3)"    },
	{  6, 14,  9, -1, 142,  2,    -1,    -1, NC, PENTIUM_      ,     0, "Kaby Lake-U (Pentium)"    },
	{  6, 14,  9, -1, 142,  2,    -1,    -1, NC, CELERON_      ,     0, "Kaby Lake-U (Celeron)"    },
	{  6, 14,  9, -1, 142,  2,    -1,    -1, NC, CORE_|_M_|_3  , _7xxx, "Kaby Lake-U (Core m3)"    },
	{  6, 14,  9, -1, 158,  4,    -1,    -1, NC, CORE_|_I_|_7  , _8xxx, "Kaby Lake-G (Core i7)"    },
	{  6, 14,  9, -1, 158,  4,    -1,    -1, NC, CORE_|_I_|_5  , _8xxx, "Kaby Lake-G (Core i5)"    },
	{  6, 14, 10, -1, 142,  4,    -1,    -1, NC, CORE_|_I_|_7  , _8xxx, "Kaby Lake-R (Core i7)"    }, /* i7-8550U + i7-8650U */
	{  6, 14, 10, -1, 142,  4,    -1,    -1, NC, CORE_|_I_|_5  , _8xxx, "Kaby Lake-R (Core i5)"    }, /* i5-8250U + i5-8350U */

	/* Coffee Lake CPUs (8th gen, 14nm): */
	{  6, 14, 10, -1, 158,  8,    -1,    -1, NC, CORE_|_I_|_9  , _8xxx, "Coffee Lake (Core i9)"    },
	{  6, 14, 10, -1, 158,  8,    -1,    -1, NC, CORE_|_I_|_7  , _8xxx, "Coffee Lake (Core i7)"    },
	{  6, 14, 10, -1, 158,  6,    -1,    -1, NC, CORE_|_I_|_7  , _8xxx, "Coffee Lake (Core i7)"    },
	{  6, 14, 10, -1, 158,  6,    -1,    -1, NC, CORE_|_I_|_5  , _8xxx, "Coffee Lake (Core i5)"    },
	{  6, 14, 10, -1, 158,  4,    -1,    -1, NC, CORE_|_I_|_3  , _8xxx, "Coffee Lake (Core i3)"    },
	{  6, 14, 10, -1, 158,  2,    -1,    -1, NC, PENTIUM_      ,     0, "Coffee Lake (Pentium)"    },
	{  6, 14, 10, -1, 158,  2,    -1,    -1, NC, CELERON_      ,     0, "Coffee Lake (Celeron)"    },
	{  6, 14, 10, -1, 142,  4,    -1,    -1, NC, CORE_|_I_|_7  , _8xxx, "Coffee Lake-U (Core i7)"  },
	{  6, 14, 10, -1, 142,  4,    -1,    -1, NC, CORE_|_I_|_5  , _8xxx, "Coffee Lake-U (Core i5)"  },
	{  6, 14, 10, -1, 142,  2,    -1,    -1, NC, CORE_|_I_|_3  , _8xxx, "Coffee Lake-U (Core i3)"  },
	{  6,  6, -1, -1, 102,  2,    -1,    -1, NC, CORE_|_I_|_3  , _8xxx, "Cannon Lake (Core i3)"    }, /* Core i3 8121U */
	{  6,  6, -1, -1, 102,  2,    -1,    -1, NC, CORE_|_M_|_3  , _8xxx, "Cannon Lake (Core m3)"    }, /* Core m3 8114Y */
	{  6, 14, 12, -1, 142,  4,    -1,    -1, NC, CORE_|_I_|_7  , _8xxx, "Whiskey Lake-U (Core i7)" },
	{  6, 14, 12, -1, 142,  4,    -1,    -1, NC, CORE_|_I_|_5  , _8xxx, "Whiskey Lake-U (Core i5)" },
	{  6, 14, 12, -1, 142,  2,    -1,    -1, NC, CORE_|_I_|_3  , _8xxx, "Whiskey Lake-U (Core i3)" },
	{  6, 14, 12, -1, 142,  2,    -1,    -1, NC, PENTIUM_      , _8xxx, "Whiskey Lake-U (Pentium)" },
	{  6, 14, 12, -1, 142,  2,    -1,    -1, NC, CELERON_      , _8xxx, "Whiskey Lake-U (Celeron)" },

	/* Coffee Lake Refresh CPUs (9th gen, 14nm): */
	{  6, 14, 13, -1, 158,  8,    -1,    -1, NC, CORE_|_I_|_9  , _9xxx, "Coffee Lake-R (Core i9)"  },
	{  6, 14, 12, -1, 158,  8,    -1,    -1, NC, CORE_|_I_|_9  , _9xxx, "Coffee Lake-R (Core i9)"  },
	{  6, 14, 13, -1, 158,  8,    -1,    -1, NC, CORE_|_I_|_7  , _9xxx, "Coffee Lake-R (Core i7)"  },
	{  6, 14, 12, -1, 158,  8,    -1,    -1, NC, CORE_|_I_|_7  , _9xxx, "Coffee Lake-R (Core i7)"  },
	{  6, 14, 13, -1, 158,  6,    -1,    -1, NC, CORE_|_I_|_5  , _9xxx, "Coffee Lake-R (Core i5)"  },
	{  6, 14, 11, -1, 158,  4,    -1,    -1, NC, CORE_|_I_|_3  , _9xxx, "Coffee Lake-R (Core i3)"  },

	/* Cascade Lake CPUs (2019, 2nd Xeon Scalable gen, 14nm) => https://en.wikichip.org/wiki/intel/microarchitectures/cascade_lake */
	{  6,  5,  7, -1, 85,  -1,    -1,    -1, NC, CORE_|_I_|_9     , _10xxx, "Cascade Lake-X (Core i9)"        },
	{  6,  5, -1, -1, 85,  -1,    -1,    -1, NC, XEON_|_W_        ,  _x2xx, "Cascade Lake-W (Xeon W)"         },
	{  6,  5, -1, -1, 85,  -1,    -1,    -1, NC, XEON_|_PLATINIUM_,  _x2xx, "Cascade Lake-SP (Xeon Platinum)" },
	{  6,  5, -1, -1, 85,  -1,    -1,    -1, NC, XEON_|_GOLD_     ,  _x2xx, "Cascade Lake-SP (Xeon Gold)"     },
	{  6,  5, -1, -1, 85,  -1,    -1,    -1, NC, XEON_|_SILVER_   ,  _x2xx, "Cascade Lake-SP (Xeon Silver)"   },
	{  6,  5, -1, -1, 85,  -1,    -1,    -1, NC, XEON_|_BRONZE_   ,  _x2xx, "Cascade Lake-SP (Xeon Bronze)"   },

	/* Comet Lake CPUs (10th gen, 14nm): */
	{  6,  5, -1, -1, 165, 10,    -1,    -1, NC, CORE_|_I_|_9,  _10xxx, "Comet Lake (Core i9)"     },
	{  6,  5, -1, -1, 165,  8,    -1,    -1, NC, CORE_|_I_|_7,  _10xxx, "Comet Lake (Core i7)"     },
	{  6,  5, -1, -1, 165,  6,    -1,    -1, NC, CORE_|_I_|_5,  _10xxx, "Comet Lake (Core i5)"     },
	{  6,  5, -1, -1, 165,  4,    -1,    -1, NC, CORE_|_I_|_3,  _10xxx, "Comet Lake (Core i3)"     },
	{  6,  5, -1, -1, 165,  2,    -1,    -1, NC, PENTIUM_    ,       0, "Comet Lake (Pentium)"     },
	{  6,  5, -1, -1, 165,  2,    -1,    -1, NC, CELERON_    ,       0, "Comet Lake (Celeron)"     },
	{  6, 14, 12, -1, 142,  6,    -1,    -1, NC, CORE_|_I_|_7,  _10xxx, "Comet Lake-U (Core i7)"   },
	{  6, 14, 12, -1, 142,  4,    -1,    -1, NC, CORE_|_I_|_7,  _10xxx, "Comet Lake-U (Core i7)"   },
	{  6, 14, 12, -1, 142,  4,    -1,    -1, NC, CORE_|_I_|_5,  _10xxx, "Comet Lake-U (Core i5)"   },
	{  6, 14, 12, -1, 142,  2,    -1,    -1, NC, PENTIUM_    ,       0, "Comet Lake-U (Pentium)"   },
	{  6, 14, 12, -1, 142,  2,    -1,    -1, NC, CELERON_    ,       0, "Comet Lake-U (Celeron)"   },

	/* Ice Lake (client) CPUs (2019, 10th Core i gen, 10nm) => https://en.wikichip.org/wiki/intel/microarchitectures/ice_lake_(client) */
	{  6, 14, -1, -1, 126,  4,    -1,    -1, NC, CORE_|_I_|_7  ,_10xxx, "Ice Lake (Core i7)"       },
	{  6, 14, -1, -1, 126,  4,    -1,    -1, NC, CORE_|_I_|_5  ,_10xxx, "Ice Lake (Core i5)"       },
	{  6, 14, -1, -1, 126,  2,    -1,    -1, NC, CORE_|_I_|_3  ,_10xxx, "Ice Lake (Core i3)"       },
	/* Ice Lake (server) CPUs (2021, 3rd Xeon Scalable gen, 10nm) => https://en.wikichip.org/wiki/intel/microarchitectures/ice_lake_(server) */
	{  6, 12, -1, -1, 108,  4,    -1,    -1, NC, XEON_            ,      0, "Ice Lake-D (Xeon-D)"         },
	{  6, 10, -1, -1, 106,  -1,   -1,    -1, NC, XEON_|_W_        ,  _x3xx, "Ice Lake-W (Xeon W)"         },
	{  6, 10, -1, -1, 106,  -1,   -1,    -1, NC, XEON_|_PLATINIUM_,  _x3xx, "Ice Lake-SP (Xeon Platinum)" },
	{  6, 10, -1, -1, 106,  -1,   -1,    -1, NC, XEON_|_GOLD_,       _x3xx, "Ice Lake-SP (Xeon Gold)"     },
	{  6, 10, -1, -1, 106,  -1,   -1,    -1, NC, XEON_|_SILVER_,     _x3xx, "Ice Lake-SP (Xeon Silver)"   },
	{  6, 10, -1, -1, 106,  -1,   -1,    -1, NC, XEON_|_BRONZE_,     _x3xx, "Ice Lake-SP (Xeon Bronze)"   },

	/* Rocket Lake CPUs (11th gen, 14nm): */
	{  6, 7, -1, -1, 167,  -1,    -1,    -1, NC, CORE_|_I_|_9  ,_11xxx, "Rocket Lake (Core i9)"    },
	{  6, 7, -1, -1, 167,  -1,    -1,    -1, NC, CORE_|_I_|_7  ,_11xxx, "Rocket Lake (Core i7)"    },
	{  6, 7, -1, -1, 167,  -1,    -1,    -1, NC, CORE_|_I_|_5  ,_11xxx, "Rocket Lake (Core i5)"    },
	{  6, 7, -1, -1, 167,  -1,    -1,    -1, NC, CORE_|_I_|_3  ,_11xxx, "Rocket Lake (Core i3)"    },
	{  6, 7, -1, -1, 167,  -1,    -1,    -1, NC, XEON_         ,     0, "Rocket Lake (Xeon-E)"     },

	/* Goldmont Plus CPUs (2017, 14nm, low-power) */
	{  6, 10, -1, -1, 122,  4,    -1,    -1, NC, PENTIUM_      ,     0, "Gemini Lake (Pentium)"    },
	{  6, 10, -1, -1, 122,  4,    -1,    -1, NC, CELERON_      ,     0, "Gemini Lake (Celeron)"    },
	{  6, 10, -1, -1, 122,  2,    -1,    -1, NC, CELERON_      ,     0, "Gemini Lake (Celeron)"    },
	{  6, 12, -1, -1,  92, -1,    -1,    -1, NC, ATOM_         ,     0, "Apollo Lake (Atom)"       },

	/* Tremont CPUs (2020, 10nm, low-power) */
	{  6,  6, -1, -1, 150, -1,    -1,    -1, NC, PENTIUM_      ,     0, "Elkhart Lake (Pentium)"   },
	{  6,  6, -1, -1, 150, -1,    -1,    -1, NC, CELERON_      ,     0, "Elkhart Lake (Celeron)"   },
	{  6,  6, -1, -1, 150, -1,    -1,    -1, NC, ATOM_         ,     0, "Elkhart Lake (Atom)"      },
	{  6, 10, -1, -1, 138, -1,    -1,    -1, NC, CORE_|_I_|_5  ,     0, "Lakefield (Core i5)"      },
	{  6, 10, -1, -1, 138, -1,    -1,    -1, NC, CORE_|_I_|_3  ,     0, "Lakefield (Core i3)"      },
	{  6, 12, -1, -1, 156, -1,    -1,    -1, NC, PENTIUM_      ,     0, "Jasper Lake (Pentium)"    },
	{  6, 12, -1, -1, 156, -1,    -1,    -1, NC, CELERON_      ,     0, "Jasper Lake (Celeron)"    },

	/* Tiger Lake CPUs (2020, 11th gen, 10nm, mobile processors): */
	{  6, 12, -1, -1, 140, -1,    -1,    -1, NC, CORE_|_I_|_9  ,_11xxx, "Tiger Lake (Core i9)"     },
	{  6, 12, -1, -1, 140, -1,    -1,    -1, NC, CORE_|_I_|_7  ,_11xxx, "Tiger Lake (Core i7)"     },
	{  6, 12, -1, -1, 140, -1,    -1,    -1, NC, CORE_|_I_|_5  ,_11xxx, "Tiger Lake (Core i5)"     },
	{  6, 12, -1, -1, 140, -1,    -1,    -1, NC, CORE_|_I_|_3  ,_11xxx, "Tiger Lake (Core i3)"     },
	{  6, 12, -1, -1, 140,  2,    -1,    -1, NC, PENTIUM_      ,     0, "Tiger Lake (Pentium)"     },
	{  6, 12, -1, -1, 140,  2,    -1,    -1, NC, CELERON_      ,     0, "Tiger Lake (Celeron)"     },

	/* Alder Lake CPUs (2021, 12th gen, 10nm) => https://en.wikichip.org/wiki/intel/microarchitectures/alder_lake */
	{  6,  7, -1, -1, 151, -1,    -1,    -1, NC, CORE_|_I_|_9  ,     _12xxx, "Alder Lake-S (Core i9)"  },
	{  6,  7, -1, -1, 151, -1,    -1,    -1, NC, CORE_|_I_|_7  ,     _12xxx, "Alder Lake-S (Core i7)"  },
	{  6,  7, -1, -1, 151, -1,    -1,    -1, NC, CORE_|_I_|_5  ,     _12xxx, "Alder Lake-S (Core i5)"  },
	{  6,  7, -1, -1, 151, -1,    -1,    -1, NC, CORE_|_I_|_3  ,     _12xxx, "Alder Lake-S (Core i3)"  },
	{  6,  7, -1, -1, 151, -1,    -1,    -1, NC, PENTIUM_      ,          0, "Alder Lake-S (Pentium)"  },
	{  6,  7, -1, -1, 151, -1,    -1,    -1, NC, CELERON_      ,          0, "Alder Lake-S (Celeron)"  },
	{  6,  7, -1, -1, 151, -1,    -1,    -1, NC, CORE_|_I_|_9|_H|_X, _12xxx, "Alder Lake-HX (Core i9)" },
	{  6,  7, -1, -1, 151, -1,    -1,    -1, NC, CORE_|_I_|_7|_H|_X, _12xxx, "Alder Lake-HX (Core i7)" },
	{  6,  7, -1, -1, 151, -1,    -1,    -1, NC, CORE_|_I_|_5|_H|_X, _12xxx, "Alder Lake-HX (Core i5)" },
	{  6, 10, -1, -1, 154, -1,    -1,    -1, NC, CORE_|_I_|_7|_P  ,       0, "Alder Lake-P (Core i7)"  },
	{  6, 10, -1, -1, 154, -1,    -1,    -1, NC, CORE_|_I_|_5|_P  ,       0, "Alder Lake-P (Core i5)"  },
	{  6, 10, -1, -1, 154, -1,    -1,    -1, NC, CORE_|_I_|_3|_P  ,       0, "Alder Lake-P (Core i3)"  },
	{  6, 10, -1, -1, 154, -1,    -1,    -1, NC, CORE_|_I_|_7|_U  ,       0, "Alder Lake-U (Core i7)"  },
	{  6, 10, -1, -1, 154, -1,    -1,    -1, NC, CORE_|_I_|_5|_U  ,       0, "Alder Lake-U (Core i5)"  },
	{  6, 10, -1, -1, 154, -1,    -1,    -1, NC, CORE_|_I_|_3|_U  ,       0, "Alder Lake-U (Core i3)"  },
	{  6, 10, -1, -1, 154, -1,    -1,    -1, NC, PENTIUM_      ,          0, "Alder Lake-U (Pentium)"  },
	{  6, 10, -1, -1, 154, -1,    -1,    -1, NC, CELERON_      ,          0, "Alder Lake-U (Celeron)"  },
	{  6, 10, -1, -1, 154, -1,    -1,    -1, NC, CORE_|_I_|_9|_H  ,  _12xxx, "Alder Lake-H (Core i9)"  },
	{  6, 10, -1, -1, 154, -1,    -1,    -1, NC, CORE_|_I_|_7|_H  ,  _12xxx, "Alder Lake-H (Core i7)"  },
	{  6, 10, -1, -1, 154, -1,    -1,    -1, NC, CORE_|_I_|_5|_H  ,  _12xxx, "Alder Lake-H (Core i5)"  },
	{  6, 14, -1, -1, 190, -1,    -1,    -1, NC, CORE_|_I_|_3|_N  ,       0, "Alder Lake-N (Core i3)"  },
	{  6, 14, -1, -1, 190, -1,    -1,    -1, NC, ATOM_            ,       0, "Alder Lake-N (Atom)"     },
	{  6, 14, -1, -1, 190,  4,    -1,    -1, NC, _N               ,       0, "Alder Lake-N"            },
	{  6, 14, -1, -1, 190,  2,    -1,    -1, NC, _N               ,       0, "Alder Lake-N"            },

	/* Raptor Lake CPUs (2022, 13th Core i gen, Intel 7) => https://en.wikichip.org/wiki/intel/microarchitectures/raptor_lake */
	{  6, 15, -1, -1, 191, -1,    -1,    -1, NC, CORE_|_I_|_5      , _13xxx, "Raptor Lake-S (Core i5)"  }, // "Golden Cove" cores
	{  6, 15, -1, -1, 191, -1,    -1,    -1, NC, CORE_|_I_|_3      , _13xxx, "Raptor Lake-S (Core i3)"  }, // "Golden Cove" cores
	{  6,  7, -1, -1, 183, -1,    -1,    -1, NC, CORE_|_I_|_9      , _13xxx, "Raptor Lake-S (Core i9)"  },
	{  6,  7, -1, -1, 183, -1,    -1,    -1, NC, CORE_|_I_|_7      , _13xxx, "Raptor Lake-S (Core i7)"  },
	{  6,  7, -1, -1, 183, -1,    -1,    -1, NC, CORE_|_I_|_5      , _13xxx, "Raptor Lake-S (Core i5)"  },
	{  6,  7, -1, -1, 183, -1,    -1,    -1, NC, CORE_|_I_|_3      , _13xxx, "Raptor Lake-S (Core i3)"  },
	{  6,  7, -1, -1, 183, -1,    -1,    -1, NC, CORE_|_I_|_9|_H|_X, _13xxx, "Raptor Lake-HX (Core i9)" },
	{  6,  7, -1, -1, 183, -1,    -1,    -1, NC, CORE_|_I_|_7|_H|_X, _13xxx, "Raptor Lake-HX (Core i7)" },
	{  6,  7, -1, -1, 183, -1,    -1,    -1, NC, CORE_|_I_|_5|_H|_X, _13xxx, "Raptor Lake-HX (Core i5)" },
	{  6, 10,  2, -1, 186, -1,    -1,    -1, NC, CORE_|_I_|_7|_P   ,      0, "Raptor Lake-P (Core i7)"  },
	{  6, 10,  2, -1, 186, -1,    -1,    -1, NC, CORE_|_I_|_5|_P   ,      0, "Raptor Lake-P (Core i5)"  },
	{  6, 10,  3, -1, 186, -1,    -1,    -1, NC, CORE_|_I_|_7|_U   ,      0, "Raptor Lake-U (Core i7)"  },
	{  6, 10,  3, -1, 186, -1,    -1,    -1, NC, CORE_|_I_|_5|_U   ,      0, "Raptor Lake-U (Core i5)"  },
	{  6, 10,  3, -1, 186, -1,    -1,    -1, NC, CORE_|_I_|_3|_U   ,      0, "Raptor Lake-U (Core i3)"  },
	{  6, 10, -1, -1, 186, -1,    -1,    -1, NC, CORE_|_I_|_9|_H   , _13xxx, "Raptor Lake-H (Core i9)"  },
	{  6, 10, -1, -1, 186, -1,    -1,    -1, NC, CORE_|_I_|_7|_H   , _13xxx, "Raptor Lake-H (Core i7)"  },
	{  6, 10, -1, -1, 186, -1,    -1,    -1, NC, CORE_|_I_|_5|_H   , _13xxx, "Raptor Lake-H (Core i5)"  },
	/* Raptor Lake Refresh CPUs (2023, 14th Core i gen, Intel 7) => https://en.wikipedia.org/wiki/Raptor_Lake#List_of_14th_generation_Raptor_Lake_processors */
	{  6,  7, -1, -1, 183, -1,    -1,    -1, NC, CORE_|_I_|_9      , _14xxx, "Raptor Lake-S (Core i9)"  },
	{  6,  7, -1, -1, 183, -1,    -1,    -1, NC, CORE_|_I_|_7      , _14xxx, "Raptor Lake-S (Core i7)"  },
	{  6,  7, -1, -1, 183, -1,    -1,    -1, NC, CORE_|_I_|_5      , _14xxx, "Raptor Lake-S (Core i5)"  },
	{  6,  7, -1, -1, 183, -1,    -1,    -1, NC, CORE_|_I_|_3      , _14xxx, "Raptor Lake-S (Core i3)"  },
	{  6,  7, -1, -1, 183, -1,    -1,    -1, NC, CORE_|_I_|_9|_H|_X, _14xxx, "Raptor Lake-HX (Core i9)" },
	{  6,  7, -1, -1, 183, -1,    -1,    -1, NC, CORE_|_I_|_7|_H|_X, _14xxx, "Raptor Lake-HX (Core i7)" },
	{  6,  7, -1, -1, 183, -1,    -1,    -1, NC, CORE_|_I_|_5|_H|_X, _14xxx, "Raptor Lake-HX (Core i5)" },

	/* Sapphire Rapids CPUs (2023, 4th Xeon Scalable gen, Intel 7) => https://en.wikichip.org/wiki/intel/microarchitectures/sapphire_rapids */
	{  6, 15, -1, -1, 143, -1,    -1,    -1, NC, XEON_|_W_|_9     , _x4xx, "Sapphire Rapids-WS (Xeon w9)"       },
	{  6, 15, -1, -1, 143, -1,    -1,    -1, NC, XEON_|_W_|_7     , _x4xx, "Sapphire Rapids-WS (Xeon w7)"       },
	{  6, 15, -1, -1, 143, -1,    -1,    -1, NC, XEON_|_W_|_5     , _x4xx, "Sapphire Rapids-WS (Xeon w5)"       },
	{  6, 15, -1, -1, 143, -1,    -1,    -1, NC, XEON_|_W_|_3     , _x4xx, "Sapphire Rapids-WS (Xeon w3)"       },
	{  6, 15, -1, -1, 143, -1,    -1,    -1, NC, XEON_|_MAX_      , _x4xx, "Sapphire Rapids-HBM (Xeon Max)"     },
	{  6, 15, -1, -1, 143, -1,    -1,    -1, NC, XEON_|_PLATINIUM_, _x4xx, "Sapphire Rapids-SP (Xeon Platinum)" },
	{  6, 15, -1, -1, 143, -1,    -1,    -1, NC, XEON_|_GOLD_     , _x4xx, "Sapphire Rapids-SP (Xeon Gold)"     },
	{  6, 15, -1, -1, 143, -1,    -1,    -1, NC, XEON_|_SILVER_   , _x4xx, "Sapphire Rapids-SP (Xeon Silver)"   },
	{  6, 15, -1, -1, 143, -1,    -1,    -1, NC, XEON_|_BRONZE_   , _x4xx, "Sapphire Rapids-SP (Xeon Bronze)"   },

	/* Emerald Rapids CPUs (2023, 5th Xeon Scalable gen, Intel 7) => https://en.wikichip.org/wiki/intel/microarchitectures/emerald_rapids */
	{  6, 15, -1, -1, 207, -1,    -1,    -1, NC, XEON_|_PLATINIUM_, _x5xx, "Emerald Rapids-SP (Xeon Platinum)" }, // Xeon Platinum (8500)
	{  6, 15, -1, -1, 207, -1,    -1,    -1, NC, XEON_|_GOLD_     , _x5xx, "Emerald Rapids-SP (Xeon Gold)"     }, // Xeon Gold (5500 and 6500)
	{  6, 15, -1, -1, 207, -1,    -1,    -1, NC, XEON_|_SILVER_   , _x5xx, "Emerald Rapids-SP (Xeon Silver)"   }, // Xeon Silver (4500)
	{  6, 15, -1, -1, 207, -1,    -1,    -1, NC, XEON_|_BRONZE_   , _x5xx, "Emerald Rapids-SP (Xeon Bronze)"   }, // Xeon Bronze (3500)

	/* Meteor Lake CPUs (2023, Core Ultra Series 1 processors, Intel 4) => https://en.wikichip.org/wiki/intel/microarchitectures/meteor_lake */
	{  6, 10, -1, -1, 170, -1,    -1,    -1, NC, CORE_|_ULTRA_|_9|_H, _1xx, "Meteor Lake-H (Core Ultra 9)" },
	{  6, 10, -1, -1, 170, -1,    -1,    -1, NC, CORE_|_ULTRA_|_7|_H, _1xx, "Meteor Lake-H (Core Ultra 7)" },
	{  6, 10, -1, -1, 170, -1,    -1,    -1, NC, CORE_|_ULTRA_|_5|_H, _1xx, "Meteor Lake-H (Core Ultra 5)" },
	{  6, 10, -1, -1, 170, -1,    -1,    -1, NC, CORE_|_ULTRA_|_7|_U, _1xx, "Meteor Lake-U (Core Ultra 7)" },
	{  6, 10, -1, -1, 170, -1,    -1,    -1, NC, CORE_|_ULTRA_|_5|_U, _1xx, "Meteor Lake-U (Core Ultra 5)" },

	/* Arrow Lake CPUs (2024, Core Ultra Series 2 processors, TSMC N3B) => https://en.wikichip.org/wiki/intel/microarchitectures/arrow_lake */
	{  6,  6, -1, -1, 198, -1,    -1,    -1, NC, CORE_|_ULTRA_|_9|_S, _2xx, "Arrow Lake-S (Core Ultra 9)" },
	{  6,  6, -1, -1, 198, -1,    -1,    -1, NC, CORE_|_ULTRA_|_7|_S, _2xx, "Arrow Lake-S (Core Ultra 7)" },
	{  6,  6, -1, -1, 198, -1,    -1,    -1, NC, CORE_|_ULTRA_|_5|_S, _2xx, "Arrow Lake-S (Core Ultra 5)" },
	{  6,  6, -1, -1, 198, -1,    -1,    -1, NC, CORE_|_ULTRA_|_3|_S, _2xx, "Arrow Lake-S (Core Ultra 3)" },
	/* F   M   S  EF   EM #cores L2$    L3$  BC       ModelBits ModelCode                                  Name */


	/* Itaniums */
	{  7, -1, -1, -1, -1,   1,    -1,    -1, NC, 0             ,     0, "Itanium"                  },
	{ 15, -1, -1, 16, -1,   1,    -1,    -1, NC, 0             ,     0, "Itanium 2"                },
};


static void load_intel_features(struct cpu_raw_data_t* raw, struct cpu_id_t* data)
{
	const struct feature_map_t matchtable_edx1[] = {
		{ 18, CPU_FEATURE_PN },
		{ 21, CPU_FEATURE_DTS },
		{ 22, CPU_FEATURE_ACPI },
		{ 27, CPU_FEATURE_SS },
		{ 29, CPU_FEATURE_TM },
		{ 30, CPU_FEATURE_IA64 },
		{ 31, CPU_FEATURE_PBE },
	};
	const struct feature_map_t matchtable_ecx1[] = {
		{  2, CPU_FEATURE_DTS64 },
		{  4, CPU_FEATURE_DS_CPL },
		{  5, CPU_FEATURE_VMX },
		{  6, CPU_FEATURE_SMX },
		{  7, CPU_FEATURE_EST },
		{  8, CPU_FEATURE_TM2 },
		{ 10, CPU_FEATURE_CID },
		{ 14, CPU_FEATURE_XTPR },
		{ 15, CPU_FEATURE_PDCM },
		{ 18, CPU_FEATURE_DCA },
		/* id 21 is handled in common */
	};
	const struct feature_map_t matchtable_eax6[] = {
		{ 0, CPU_FEATURE_INTEL_DTS },
		{ 6, CPU_FEATURE_INTEL_PTM },
	};
	const struct feature_map_t matchtable_edx81[] = {
		{ 20, CPU_FEATURE_XD },
	};
	const struct feature_map_t matchtable_ebx7[] = {
		{  2, CPU_FEATURE_SGX },
		{  4, CPU_FEATURE_HLE },
		{ 11, CPU_FEATURE_RTM },
		/* id 16 to 19 are handled in common */
		{ 26, CPU_FEATURE_AVX512PF },
		{ 27, CPU_FEATURE_AVX512ER },
		/* id 28 to 31 are handled in common */
	};

	if (raw->basic_cpuid[0][EAX] >= 1) {
		match_features(matchtable_edx1, COUNT_OF(matchtable_edx1), raw->basic_cpuid[1][EDX], data);
		match_features(matchtable_ecx1, COUNT_OF(matchtable_ecx1), raw->basic_cpuid[1][ECX], data);
	}
	if (raw->ext_cpuid[0][EAX] >= 1) {
		match_features(matchtable_edx81, COUNT_OF(matchtable_edx81), raw->ext_cpuid[1][EDX], data);
	}
	// detect INTEL DTM/PTM:
	if (raw->basic_cpuid[0][EAX] >= 6) {
		match_features(matchtable_eax6, COUNT_OF(matchtable_eax6), raw->basic_cpuid[6][EAX], data);
	}
	// detect TSX/AVX512:
	if (raw->basic_cpuid[0][EAX] >= 7) {
		match_features(matchtable_ebx7, COUNT_OF(matchtable_ebx7), raw->basic_cpuid[7][EBX], data);
	}
}

static void decode_intel_oldstyle_cache_info(struct cpu_raw_data_t* raw, struct cpu_id_t* data)
{
	uint8_t f[256] = {0};
	int reg, off;
	uint32_t x;
	for (reg = 0; reg < 4; reg++) {
		x = raw->basic_cpuid[2][reg];
		if (x & 0x80000000) continue;
		for (off = 0; off < 4; off++) {
			f[x & 0xff] = 1;
			x >>= 8;
		}
	}

	assign_cache_data(f[0x06], L1I,      8,  4,  32, data);
	assign_cache_data(f[0x08], L1I,     16,  4,  32, data);
	assign_cache_data(f[0x0A], L1D,      8,  2,  32, data);
	assign_cache_data(f[0x0C], L1D,     16,  4,  32, data);
	assign_cache_data(f[0x22],  L3,    512,  4,  64, data);
	assign_cache_data(f[0x23],  L3,   1024,  8,  64, data);
	assign_cache_data(f[0x25],  L3,   2048,  8,  64, data);
	assign_cache_data(f[0x29],  L3,   4096,  8,  64, data);
	assign_cache_data(f[0x2C], L1D,     32,  8,  64, data);
	assign_cache_data(f[0x30], L1I,     32,  8,  64, data);
	assign_cache_data(f[0x39],  L2,    128,  4,  64, data);
	assign_cache_data(f[0x3A],  L2,    192,  6,  64, data);
	assign_cache_data(f[0x3B],  L2,    128,  2,  64, data);
	assign_cache_data(f[0x3C],  L2,    256,  4,  64, data);
	assign_cache_data(f[0x3D],  L2,    384,  6,  64, data);
	assign_cache_data(f[0x3E],  L2,    512,  4,  64, data);
	assign_cache_data(f[0x41],  L2,    128,  4,  32, data);
	assign_cache_data(f[0x42],  L2,    256,  4,  32, data);
	assign_cache_data(f[0x43],  L2,    512,  4,  32, data);
	assign_cache_data(f[0x44],  L2,   1024,  4,  32, data);
	assign_cache_data(f[0x45],  L2,   2048,  4,  32, data);
	assign_cache_data(f[0x46],  L3,   4096,  4,  64, data);
	assign_cache_data(f[0x47],  L3,   8192,  8,  64, data);
	assign_cache_data(f[0x4A],  L3,   6144, 12,  64, data);
	assign_cache_data(f[0x4B],  L3,   8192, 16,  64, data);
	assign_cache_data(f[0x4C],  L3,  12288, 12,  64, data);
	assign_cache_data(f[0x4D],  L3,  16384, 16,  64, data);
	assign_cache_data(f[0x4E],  L2,   6144, 24,  64, data);
	assign_cache_data(f[0x60], L1D,     16,  8,  64, data);
	assign_cache_data(f[0x66], L1D,      8,  4,  64, data);
	assign_cache_data(f[0x67], L1D,     16,  4,  64, data);
	assign_cache_data(f[0x68], L1D,     32,  4,  64, data);
	/* The following four entries are trace cache. Intel does not
	 * specify a cache-line size, so we use -1 instead
	 */
	assign_cache_data(f[0x70], L1I,     12,  8,  -1, data);
	assign_cache_data(f[0x71], L1I,     16,  8,  -1, data);
	assign_cache_data(f[0x72], L1I,     32,  8,  -1, data);
	assign_cache_data(f[0x73], L1I,     64,  8,  -1, data);

	assign_cache_data(f[0x78],  L2,   1024,  4,  64, data);
	assign_cache_data(f[0x79],  L2,    128,  8,  64, data);
	assign_cache_data(f[0x7A],  L2,    256,  8,  64, data);
	assign_cache_data(f[0x7B],  L2,    512,  8,  64, data);
	assign_cache_data(f[0x7C],  L2,   1024,  8,  64, data);
	assign_cache_data(f[0x7D],  L2,   2048,  8,  64, data);
	assign_cache_data(f[0x7F],  L2,    512,  2,  64, data);
	assign_cache_data(f[0x82],  L2,    256,  8,  32, data);
	assign_cache_data(f[0x83],  L2,    512,  8,  32, data);
	assign_cache_data(f[0x84],  L2,   1024,  8,  32, data);
	assign_cache_data(f[0x85],  L2,   2048,  8,  32, data);
	assign_cache_data(f[0x86],  L2,    512,  4,  64, data);
	assign_cache_data(f[0x87],  L2,   1024,  8,  64, data);

	if (f[0x49]) {
		/* This flag is overloaded with two meanings. On Xeon MP
		 * (family 0xf, model 0x6) this means L3 cache. On all other
		 * CPUs (notably Conroe et al), this is L2 cache. In both cases
		 * it means 4MB, 16-way associative, 64-byte line size.
		 */
		if (data->x86.family == 0xf && data->x86.model == 0x6) {
			data->l3_cache = 4096;
			data->l3_assoc = 16;
			data->l3_cacheline = 64;
		} else {
			data->l2_cache = 4096;
			data->l2_assoc = 16;
			data->l2_cacheline = 64;
		}
	}
	if (f[0x40]) {
		/* Again, a special flag. It means:
		 * 1) If no L2 is specified, then CPU is w/o L2 (0 KB)
		 * 2) If L2 is specified by other flags, then, CPU is w/o L3.
		 */
		if (data->l2_cache == -1) {
			data->l2_cache = 0;
		} else {
			data->l3_cache = 0;
		}
	}
}

static int decode_intel_extended_topology(struct cpu_raw_data_t* raw, struct cpu_id_t* data)
{
	int i, level_type, num_smt = -1, num_core = -1;

	for (i = 0; (i < MAX_INTELFN11_LEVEL) && (raw->intel_fn11[i][EAX] != 0x0) && (raw->intel_fn11[i][EBX] != 0x0); i++) {
		level_type = EXTRACTS_BITS(raw->intel_fn11[i][ECX], 15, 8);
		switch (level_type) {
			case 0x01:
				num_smt = EXTRACTS_BITS(raw->intel_fn11[i][EBX], 15, 0);
				break;
			case 0x02:
				num_core = EXTRACTS_BITS(raw->intel_fn11[i][EBX], 15, 0);
				break;
			default:
				break;
		}
	}
	if (num_smt == -1 || num_core == -1) return 0;
	data->num_logical_cpus = num_core;
	data->num_cores = num_core / num_smt;
	// make sure num_cores is at least 1. In VMs, the CPUID instruction
	// is rigged and may give nonsensical results, but we should at least
	// avoid outputs like data->num_cores == 0.
	if (data->num_cores <= 0) data->num_cores = 1;
	return 1;
}

static intel_code_and_bits_t get_brand_code_and_bits(struct cpu_id_t* data)
{
	intel_code_t code = (intel_code_t) NC;
	intel_code_and_bits_t result;
	uint64_t bits = 0;
	int i = 0;
	const char* bs = data->brand_str;
	const char* s;
	const size_t n = strlen(bs);
	const struct { intel_code_t c; const char *search; } matchtable[] = {
		{ PENTIUM_M, "Pentium(R) M" },
		{ CORE_SOLO, "Pentium(R) Dual  CPU" },
		{ CORE_SOLO, "Pentium(R) Dual-Core" },
		{ PENTIUM_D, "Pentium(R) D" },
		{ CORE_SOLO, "Genuine Intel(R) CPU" },
		{ CORE_SOLO, "Intel(R) Core(TM)" },
		{ DIAMONDVILLE, "CPU [N ][23]## " },
		{ SILVERTHORNE, "CPU Z" },
		{ PINEVIEW, "CPU [ND][45]## " },
		{ CEDARVIEW, "CPU [ND]#### " },
	};

	const struct { uint64_t bit; const char* search; } bit_matchtable[] = {
		{ XEON_, "Xeon" },
		{ _MP_, " MP" },
		{ ATOM_, "Atom" },
		{ MOBILE_, "Mobile" },
		{ CELERON_, "Celeron" },
		{ PENTIUM_, "Pentium" },
		{ _BRONZE_, "Bronze" },
		{ _SILVER_, "Silver" },
		{ _GOLD_, "Gold" },
		{ _PLATINIUM_, "Platinum" },
		{ _MAX_, "Max" },
	};

	for (i = 0; i < COUNT_OF(bit_matchtable); i++) {
		if (match_pattern(bs, bit_matchtable[i].search))
			bits |= bit_matchtable[i].bit;
	}

	if ((i = match_pattern(bs, "Core(TM) [im][3579]")) != 0) {
		bits |= CORE_;
		i--;
		switch (bs[i + 9]) {
			case 'i': bits |= _I_; break;
			case 'm': bits |= _M_; break;
		}
		switch (bs[i + 10]) {
			case '3': bits |= _3; break;
			case '5': bits |= _5; break;
			case '7': bits |= _7; break;
			case '9': bits |= _9; break;
		}
		for(i = i + 11; i < (int)n; i++) {
			switch (bs[i]) {
				case 'H': bits |= _H; break;
				case 'K': bits |= _K; break;
				case 'N': bits |= _N; break;
				case 'P': bits |= _P; break;
				case 'S': bits |= _S; break;
				case 'U': bits |= _U; break;
				case 'X': bits |= _X; break;
			}
		}
	}
	if ((i = match_pattern(bs, "Core(TM) Ultra [579]")) != 0) {
		bits |= CORE_ | _ULTRA_;
		i--;
		switch (bs[i + 15]) {
			//case '3': bits |= _3; break;
			case '5': bits |= _5; break;
			case '7': bits |= _7; break;
			case '9': bits |= _9; break;
		}
		for(i = i + 16; i < (int)n; i++) {
			switch (bs[i]) {
				case 'H': bits |= _H; break;
				//case 'K': bits |= _K; break;
				//case 'N': bits |= _N; break;
				//case 'P': bits |= _P; break;
				//case 'S': bits |= _S; break;
				case 'U': bits |= _U; break;
				//case 'X': bits |= _X; break;
			}
		}
	}
	else if ((i = match_pattern(bs, "Xeon(R) w[3579]")) != 0) {
		bits |= XEON_;
		i--;
		switch (bs[i + 8]) {
			case 'w': bits |= _W_; break;
		}
		switch (bs[i + 9]) {
			case '3': bits |= _3; break;
			case '5': bits |= _5; break;
			case '7': bits |= _7; break;
			case '9': bits |= _9; break;
		}
	}
	else if ((i = match_pattern(bs, "Xeon(R) [DW]")) != 0) {
		bits |= XEON_;
		i--;
		switch (bs[i + 8]) {
			case 'D': bits |= _D_; break;
			case 'W': bits |= _W_; break;
		}
	}

	if (((bits & PENTIUM_) || (bits & CELERON_)) && ((i = match_pattern(bs, "[JN]")) != 0)) {
		i--;
		switch (bs[i]) {
			case 'J': bits |= _J_; break;
			case 'N': bits |= _N_; break;
		}
	}

	for (i = 0; i < COUNT_OF(matchtable); i++)
		if (match_pattern(bs, matchtable[i].search)) {
			code = matchtable[i].c;
			break;
		}
	debugf(2, "intel matchtable result is %d\n", code);
	if (bits & XEON_) {
		if (match_pattern(bs, "W35##") || match_pattern(bs, "[ELXW]75##"))
			bits |= _7;
		else if (match_pattern(bs, "[ELXW]55##"))
			code = GAINESTOWN;
		else if (match_pattern(bs, "[ELXW]56##"))
			code = WESTMERE;
		else if (data->l3_cache > 0 && data->x86.family == 16)
			/* restrict by family, since later Xeons also have L3 ... */
			code = IRWIN;
	}
	if (match_all(bits, XEON_ + _MP_) && data->l3_cache > 0)
		code = POTOMAC;
	if (code == CORE_SOLO) {
		s = strstr(bs, "CPU");
		if (s) {
			s += 3;
			while (*s == ' ') s++;
			if (*s == 'T')
				bits |= MOBILE_;
		}
	}
	if (code == CORE_SOLO) {
		switch (data->num_cores) {
			case 1: break;
			case 2:
			{
				code = CORE_DUO;
				if (data->num_logical_cpus > 2)
					code = DUAL_CORE_HT;
				break;
			}
			case 4:
			{
				code = QUAD_CORE;
				if (data->num_logical_cpus > 4)
					code = QUAD_CORE_HT;
				break;
			}
			default:
				code = MORE_THAN_QUADCORE; break;
		}
	}

	if (code == CORE_DUO && (bits & MOBILE_) && data->x86.model != 14) {
		if (data->x86.ext_model < 23) {
			code = MEROM;
		} else {
			code = PENRYN;
		}
	}
	if (data->x86.ext_model == 23 &&
		(code == CORE_DUO || code == PENTIUM_D || (bits & CELERON_))) {
		code = WOLFDALE;
	}

	result.code = code;
	result.bits = bits;
	return result;
}

static intel_model_t get_model_code(struct cpu_id_t* data)
{
	int i = 0;
	int l = (int) strlen(data->brand_str);
	const char *bs = data->brand_str;
	int mod_flags = 0, model_no = 0, ndigs = 0;
	/* If the CPU is a Core ix, then just return the model number generation: */
	if ((i = match_pattern(bs, "Core(TM) i[3579]")) != 0) {
		i += 11;
		if (i + 4 >= l) return UNKNOWN;
		if (bs[i] == '2') return _2xxx;
		if (bs[i] == '3') return _3xxx;
		if (bs[i] == '4') return _4xxx;
		if (bs[i] == '5') return _5xxx;
		if (bs[i] == '6') return _6xxx;
		if (bs[i] == '7') return _7xxx;
		if (bs[i] == '8') return _8xxx;
		if (bs[i] == '9') return _9xxx;
		if ((bs[i] == '1') && (bs[i+1] == '0')) return _10xxx;
		if ((bs[i] == '1') && (bs[i+1] == '1')) return _11xxx;
		if ((bs[i] == '1') && (bs[i+1] == '2')) return _12xxx;
		if ((bs[i] == '1') && (bs[i+1] == '3')) return _13xxx;
		if ((bs[i] == '1') && (bs[i+1] == '4')) return _14xxx;
		return UNKNOWN;
	}
	else if ((i = match_pattern(bs, "Core(TM) Ultra [3579]")) != 0) {
		i += 16;
		if (i + 3 >= l) return UNKNOWN;
		if (bs[i] == '1') return _1xx;
		if (bs[i] == '2') return _2xx;
		return UNKNOWN;
	}
	else if ((i = match_pattern(bs, "Xeon(R) [WBSGP]")) != 0) {
		i = 0;
		if                   ((i = match_pattern(bs, "Xeon(R) W-"))       != 0)  i += 10;
		else if ((i == 0) && ((i = match_pattern(bs, "Xeon(R) Bronze"))   != 0)) i += 15;
		else if ((i == 0) && ((i = match_pattern(bs, "Xeon(R) Silver"))   != 0)) i += 15;
		else if ((i == 0) && ((i = match_pattern(bs, "Xeon(R) Gold"))     != 0)) i += 13;
		else if ((i == 0) && ((i = match_pattern(bs, "Xeon(R) Platinum")) != 0)) i += 17;
		else if ((i == 0) && ((i = match_pattern(bs, "Xeon(R) Max"))      != 0)) i += 12;

		if (i == 0) return UNKNOWN;
		if (bs[i] == '1') return _x1xx;
		if (bs[i] == '2') return _x2xx;
		if (bs[i] == '3') return _x3xx;
		if (bs[i] == '4') return _x4xx;
		if (bs[i] == '5') return _x5xx;
		return UNKNOWN;
	}

	/* For Core2-based Xeons: */
	while (i < l - 3) {
		if (bs[i] == 'C' && bs[i+1] == 'P' && bs[i+2] == 'U')
			break;
		i++;
	}
	if (i >= l - 3) return UNKNOWN;
	i += 3;
	while (i < l - 4 && bs[i] == ' ') i++;
	if (i >= l - 4) return UNKNOWN;
	while (i < l - 4 && !isdigit(bs[i])) {
		if (bs[i] >= 'A' && bs[i] <= 'Z')
			mod_flags |= (1 << (bs[i] - 'A'));
		i++;
	}
	if (i >= l - 4) return UNKNOWN;
	while (isdigit(bs[i])) {
		ndigs++;
		model_no = model_no * 10 + (int) (bs[i] - '0');
		i++;
	}
	if (ndigs != 4) return UNKNOWN;
#define HAVE(ch, flags) ((flags & (1 << ((int)(ch-'A')))) != 0)
	switch (model_no / 100) {
		case 30: return _3000;
		case 31: return _3100;
		case 32:
		{
			return (HAVE('X', mod_flags)) ? X3200 : _3200;
		}
		case 33:
		{
			return (HAVE('X', mod_flags)) ? X3300 : _3300;
		}
		case 51: return _5100;
		case 52: return _5200;
		case 53: return _5300;
		case 54: return _5400;
		default:
			return UNKNOWN;
	}
#undef HAVE
}

static void decode_intel_sgx_features(const struct cpu_raw_data_t* raw, struct cpu_id_t* data)
{
	struct cpu_epc_t epc;
	int i;

	if (raw->basic_cpuid[0][EAX] < 0x12) return; // no 12h leaf
	if (raw->basic_cpuid[0x12][EAX] == 0) return; // no sub-leafs available, probably it's disabled by BIOS

	// decode sub-leaf 0:
	if (raw->basic_cpuid[0x12][EAX] & 1) data->x86.sgx.flags[INTEL_SGX1] = 1;
	if (raw->basic_cpuid[0x12][EAX] & 2) data->x86.sgx.flags[INTEL_SGX2] = 1;
	if (data->x86.sgx.flags[INTEL_SGX1] || data->x86.sgx.flags[INTEL_SGX2])
		data->x86.sgx.present = 1;
	data->x86.sgx.misc_select = raw->basic_cpuid[0x12][EBX];
	data->x86.sgx.max_enclave_32bit = (raw->basic_cpuid[0x12][EDX]     ) & 0xff;
	data->x86.sgx.max_enclave_64bit = (raw->basic_cpuid[0x12][EDX] >> 8) & 0xff;

	// decode sub-leaf 1:
	data->x86.sgx.secs_attributes = raw->intel_fn12h[1][EAX] | (((uint64_t) raw->intel_fn12h[1][EBX]) << 32);
	data->x86.sgx.secs_xfrm       = raw->intel_fn12h[1][ECX] | (((uint64_t) raw->intel_fn12h[1][EDX]) << 32);

	// decode higher-order subleafs, whenever present:
	data->x86.sgx.num_epc_sections = -1;
	for (i = 0; i < 1000000; i++) {
		epc = cpuid_get_epc(i, raw);
		if (epc.length == 0) {
			debugf(2, "SGX: epc section request for %d returned null, no more EPC sections.\n", i);
			data->x86.sgx.num_epc_sections = i;
			break;
		}
	}
	if (data->x86.sgx.num_epc_sections == -1) {
		debugf(1, "SGX: warning: seems to be infinitude of EPC sections.\n");
		data->x86.sgx.num_epc_sections = 1000000;
	}
}

struct cpu_epc_t cpuid_get_epc(int index, const struct cpu_raw_data_t* raw)
{
	uint32_t regs[4];
	struct cpu_epc_t retval = {0, 0};
	if (raw && index < MAX_INTELFN12H_LEVEL - 2) {
		// this was queried already, use the data:
		memcpy(regs, raw->intel_fn12h[2 + index], sizeof(regs));
	} else {
		// query this ourselves:
		regs[0] = 0x12;
		regs[2] = 2 + index;
		regs[1] = regs[3] = 0;
		cpu_exec_cpuid_ext(regs);
	}

	// decode values:
	if ((regs[0] & 0xf) == 0x1) {
		retval.start_addr |= (regs[0] & 0xfffff000); // bits [12, 32) -> bits [12, 32)
		retval.start_addr |= ((uint64_t) (regs[1] & 0x000fffff)) << 32; // bits [0, 20) -> bits [32, 52)
		retval.length     |= (regs[2] & 0xfffff000); // bits [12, 32) -> bits [12, 32)
		retval.length     |= ((uint64_t) (regs[3] & 0x000fffff)) << 32; // bits [0, 20) -> bits [32, 52)
	}
	return retval;
}

int cpuid_identify_intel(struct cpu_raw_data_t* raw, struct cpu_id_t* data, struct internal_id_info_t* internal)
{
	intel_code_and_bits_t brand;
	intel_model_t model_code;
	int i;
	char* brand_code_str = NULL;

	load_intel_features(raw, data);
	if (raw->basic_cpuid[0][EAX] >= 4) {
		/* Deterministic way is preferred, being more generic */
		decode_deterministic_cache_info_x86(raw->intel_fn4, MAX_INTELFN4_LEVEL, data, internal);
	} else if (raw->basic_cpuid[0][EAX] >= 2) {
		decode_intel_oldstyle_cache_info(raw, data);
	}
	if ((raw->basic_cpuid[0][EAX] < 11) || (decode_intel_extended_topology(raw, data) == 0))
		decode_number_of_cores_x86(raw, data);
	data->purpose = cpuid_identify_purpose_intel(raw);
	decode_architecture_version_x86(data);

	brand = get_brand_code_and_bits(data);
	model_code = get_model_code(data);
	for (i = 0; i < COUNT_OF(intel_bcode_str); i++) {
		if (brand.code == intel_bcode_str[i].code) {
			brand_code_str = intel_bcode_str[i].str;
			break;
		}
	}
	if (brand_code_str)
		debugf(2, "Detected Intel brand code: %d (%s)\n", brand.code, brand_code_str);
	else
		debugf(2, "Detected Intel brand code: %d\n", brand.code);
	if (brand.bits) {
		debugf(2, "Detected Intel bits: ");
		debug_print_lbits(2, brand.bits);
	}
	debugf(2, "Detected Intel model code: %d\n", model_code);

	internal->code.intel = brand.code;
	internal->bits = brand.bits;

	if (data->flags[CPU_FEATURE_SGX]) {
		debugf(2, "SGX seems to be present, decoding...\n");
		// if SGX is indicated by the CPU, verify its presence:
		decode_intel_sgx_features(raw, data);
	}

	internal->score = match_cpu_codename(cpudb_intel, COUNT_OF(cpudb_intel), data,
		brand.code, brand.bits, model_code);
	return 0;
}

cpu_purpose_t cpuid_identify_purpose_intel(struct cpu_raw_data_t* raw)
{
	/* Check for hybrid architecture
	From Intel® 64 and IA-32 Architectures Software Developer’s Manual Combined Volumes: 1, 2A, 2B, 2C, 2D, 3A, 3B, 3C, 3D, and 4
	Available at https://cdrdv2.intel.com/v1/dl/getContent/671200

	- CPUID[7h] is Structured Extended Feature Flags Enumeration Leaf (Output depends on ECX input value)
	  EDX, bit 15: Hybrid. If 1, the processor is identified as a hybrid part.

	- CPUID[1Ah] is Hybrid Information Enumeration Leaf (EAX = 1AH, ECX = 0)
	  EAX, bits 31-24: Core type
	*/
	if (EXTRACTS_BIT(raw->basic_cpuid[0x7][EDX], 15) == 0x1) {
		debugf(3, "Detected Intel CPU hybrid architecture\n");
		switch (EXTRACTS_BITS(raw->basic_cpuid[0x1a][EAX], 31, 24)) {
			case 0x20: /* Atom */
				/* Acccording to Ramyer M. from Intel, LP E-Cores do not have a L3 cache
				   https://community.intel.com/t5/Processors/Detecting-LP-E-Cores-on-Meteor-Lake-in-software/m-p/1584555/highlight/true#M70732
				   If sub-leaf 3 is set, it is an E-Cores.
				*/
				return (EXTRACTS_BITS(raw->intel_fn4[3][EAX], 31, 0)) ? PURPOSE_EFFICIENCY : PURPOSE_LP_EFFICIENCY;
			case 0x40: /* Core */
				return PURPOSE_PERFORMANCE;
			default:
				return PURPOSE_GENERAL;
		}
	}

	return PURPOSE_GENERAL;
}

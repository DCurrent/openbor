/*
 * OpenBOR - http://www.LavaLit.com
 * -----------------------------------------------------------------------
 * Licensed under the BSD license, see LICENSE in OpenBOR root for details.
 *
 * Copyright (c) 2004 - 2011 OpenBOR Team
 */

/*
ADPCM coder/decoder
*/

/* Intel ADPCM step variation table */
static int indexTable[16] = {
	-1, -1, -1, -1, 2, 4, 6, 8,
	-1, -1, -1, -1, 2, 4, 6, 8,
};

static int stepsizeTable[89] = {
	7, 8, 9, 10, 11, 12, 13, 14, 16, 17,
	19, 21, 23, 25, 28, 31, 34, 37, 41, 45,
	50, 55, 60, 66, 73, 80, 88, 97, 107, 118,
	130, 143, 157, 173, 190, 209, 230, 253, 279, 307,
	337, 371, 408, 449, 494, 544, 598, 658, 724, 796,
	876, 963, 1060, 1166, 1282, 1411, 1552, 1707, 1878, 2066,
	2272, 2499, 2749, 3024, 3327, 3660, 4026, 4428, 4871, 5358,
	5894, 6484, 7132, 7845, 8630, 9493, 10442, 11487, 12635, 13899,
	15289, 16818, 18500, 20350, 22385, 24623, 27086, 29794, 32767
};

struct adpcm_state {
	int	valprev[2];	/* Previous output value */
	int	index[2];	/* Index into stepsize table */
};
static struct adpcm_state state;

void adpcm_reset(){
	state.valprev[0] = 0;
	state.valprev[1] = 0;
	state.index[0] = 0;
	state.index[1] = 0;
}

short adpcm_valprev(int channel)
{
	return state.valprev[channel];
}

unsigned char adpcm_index(int channel)
{
	return state.index[channel];
}

void adpcm_loop_reset(int channel, short valprev, unsigned char index)
{
	state.valprev[channel] = valprev;
	state.index[channel] = index;
}

// len = input buffer size in bytes
// This should always be a multiple of 2 for 16-bit data.
// Return value: number of encoded bytes.
int adpcm_encode_mono(short * indata, unsigned char * outdata, int len){
	int val = 0;				/* Current input sample value */
	int sign = 0;				/* Current adpcm sign bit */
	int delta = 0;				/* Current adpcm output value */
	int diff = 0;				/* Difference between val and valprev */
	int step = 0;				/* Stepsize */
	int vpdiff = 0;				/* Current change to valpred */
	int outputbuffer = 0;		/* place to keep previous 4-bit value */
	int bufferstep = 0;			/* toggle between outputbuffer/output */
	int bytescoded = 0;

	if ( !indata || !outdata || len < 2 ) return 0;

	len /= 2;
	step = stepsizeTable[state.index[0]];
	bufferstep = 1;

	for ( ; len > 0; len-- ) {
		val = *indata++;

		/* Step 1 - compute difference with previous value */
		diff = val - state.valprev[0];
		sign = (diff < 0) ? 8 : 0;
		if ( sign ) diff = (-diff);

		/* Step 2 - Divide and clamp */
		/* Note:
		** This code *approximately* computes:
		**    delta = diff*4/step;
		**    vpdiff = (delta+0.5)*step/4;
		** but in shift step bits are dropped. The net result of this is
		** that even if you have fast mul/div hardware you cannot put it to
		** good use since the fixup would be too expensive.
		*/
		delta = 0;
		vpdiff = (step >> 3);

		if ( diff >= step ) {
			delta = 4;
			diff -= step;
			vpdiff += step;
		}
		step >>= 1;
		if ( diff >= step  ) {
			delta |= 2;
			diff -= step;
			vpdiff += step;
		}
		step >>= 1;
		if ( diff >= step ) {
			delta |= 1;
			vpdiff += step;
		}

		/* Step 3 - Update previous value */
		if ( sign )
			state.valprev[0] -= vpdiff;
		else
			state.valprev[0] += vpdiff;

		/* Step 4 - Clamp previous value to 16 bits */
		if ( state.valprev[0] > 32767 )
			state.valprev[0] = 32767;
		else if ( state.valprev[0] < -32768 )
			state.valprev[0] = -32768;

		/* Step 5 - Assemble value, update index and step values */
		delta |= sign;

		state.index[0] += indexTable[delta];
		if ( state.index[0] < 0 ) state.index[0] = 0;
		if ( state.index[0] > 88 ) state.index[0] = 88;
		step = stepsizeTable[state.index[0]];

		/* Step 6 - Output value */
		if ( bufferstep ) {
			outputbuffer = (delta << 4) & 0xf0;
		} else {
			*outdata++ = (delta & 0x0f) | outputbuffer;
			bytescoded++;
		}
		bufferstep = !bufferstep;
	}

	/* Output last step, if needed */
	if ( !bufferstep ) {
		*outdata++ = outputbuffer;
		bytescoded++;
	}

	return bytescoded;
}

// len = input buffer size in bytes
int adpcm_decode_mono(unsigned char * indata, short * outdata, int len){
	int sign = 0;				/* Current adpcm sign bit */
	int delta = 0;				/* Current adpcm output value */
	int step = 0;				/* Stepsize */
	int valpred = 0;			/* Predicted value */
	int vpdiff = 0;				/* Current change to valpred */
	int index = 0;				/* Current step change index */
	int inputbuffer = 0;		/* place to keep next 4-bit value */
	int bytesdecoded = 0;

	if ( !indata || !outdata || len < 1 ) return 0;

	len *= 2;
	valpred = state.valprev[0];
	index = state.index[0];
	step = stepsizeTable[index];

	for ( bytesdecoded = 0 ; bytesdecoded < len; bytesdecoded++ ) {

		/* Step 1 - get the delta value */
		if ( bytesdecoded & 1 ) {
			delta = inputbuffer & 0xf;
		} else {
			inputbuffer = *indata++;
			delta = (inputbuffer >> 4) & 0xf;
		}

		/* Step 2 - Find new index value (for later) */
		index += indexTable[delta];
		if ( index < 0 ) index = 0;
		if ( index > 88 ) index = 88;

		/* Step 3 - Separate sign and magnitude */
		sign = delta & 8;
		delta = delta & 7;

		/* Step 4 - Compute difference and new predicted value */
		/*
		** Computes 'vpdiff = (delta+0.5)*step/4', but see comment
		** in adpcm_coder.
		*/
		vpdiff = step >> 3;
		if ( delta & 4 ) vpdiff += step;
		if ( delta & 2 ) vpdiff += step>>1;
		if ( delta & 1 ) vpdiff += step>>2;

		if ( sign ) valpred -= vpdiff;
		else valpred += vpdiff;

		/* Step 5 - clamp output value */
		if ( valpred > 32767 ) valpred = 32767;
		else if ( valpred < -32768 ) valpred = -32768;

		/* Step 6 - Update step value */
		step = stepsizeTable[index];

		/* Step 7 - Output value */
		*outdata++ = valpred;
	}

	state.valprev[0] = valpred;
	state.index[0] = index;

	return bytesdecoded * 2;
}

// len = input buffer size in bytes
// This should always be a multiple of 4 for 16-bit stereo data.
// Return value: number of encoded bytes.
int adpcm_encode_stereo(short * indata, unsigned char * outdata, int len){
	int val = 0;				/* Current input sample value */
	int sign = 0;				/* Current adpcm sign bit */
	int delta = 0;				/* Current adpcm output value */
	int diff = 0;				/* Difference between val and valprev */
	int step = 0;				/* Stepsize */
	int vpdiff = 0;				/* Current change to valpred */
	int outputbuffer = 0;		/* place to keep previous 4-bit value */
	int bytescoded = 0;

	if( !indata || !outdata || len < 4 ) return 0;

	len /= 4;

	for ( bytescoded = 0; bytescoded < len; bytescoded++ ){

		/* Left Channel */
		step = stepsizeTable[state.index[0]];

		val = *indata++;

		/* Step 1 - compute difference with previous value */
		diff = val - state.valprev[0];
		sign = (diff < 0) ? 8 : 0;
		if ( sign ) diff = (-diff);

		/* Step 2 - Divide and clamp */
		/* Note:
		** This code *approximately* computes:
		**    delta = diff*4/step;
		**    vpdiff = (delta+0.5)*step/4;
		** but in shift step bits are dropped. The net result of this is
		** that even if you have fast mul/div hardware you cannot put it to
		** good use since the fixup would be too expensive.
		*/
		delta = 0;
		vpdiff = (step >> 3);

		if ( diff >= step ) {
			delta = 4;
			diff -= step;
			vpdiff += step;
		}
		step >>= 1;
		if ( diff >= step ) {
			delta |= 2;
			diff -= step;
			vpdiff += step;
		}
		step >>= 1;
		if ( diff >= step ) {
			delta |= 1;
			vpdiff += step;
		}

		/* Step 3 - Update previous value */
		if ( sign )
			state.valprev[0] -= vpdiff;
		else
			state.valprev[0] += vpdiff;

		/* Step 4 - Clamp previous value to 16 bits */
		if ( state.valprev[0] > 32767 )
			state.valprev[0] = 32767;
		else if ( state.valprev[0] < -32768 )
			state.valprev[0] = -32768;

		/* Step 5 - Assemble value, update index and step values */
		delta |= sign;

		state.index[0] += indexTable[delta];
		if ( state.index[0] < 0 ) state.index[0] = 0;
		if ( state.index[0] > 88 ) state.index[0] = 88;
		step = stepsizeTable[state.index[0]];

		/* Step 6 - Output value */
		outputbuffer = (delta << 4) & 0xf0;

		/* Right Channel */
		step = stepsizeTable[state.index[1]];

		val = *indata++;

		/* Step 1 - compute difference with previous value */
		diff = val - state.valprev[1];
		sign = (diff < 0) ? 8 : 0;
		if ( sign ) diff = (-diff);

		/* Step 2 - Divide and clamp */
		/* Note:
		** This code *approximately* computes:
		**    delta = diff*4/step;
		**    vpdiff = (delta+0.5)*step/4;
		** but in shift step bits are dropped. The net result of this is
		** that even if you have fast mul/div hardware you cannot put it to
		** good use since the fixup would be too expensive.
		*/
		delta = 0;
		vpdiff = (step >> 3);

		if ( diff >= step ) {
			delta = 4;
			diff -= step;
			vpdiff += step;
		}
		step >>= 1;
		if ( diff >= step ) {
			delta |= 2;
			diff -= step;
			vpdiff += step;
		}
		step >>= 1;
		if ( diff >= step ) {
			delta |= 1;
			vpdiff += step;
		}

		/* Step 3 - Update previous value */
		if ( sign )
			state.valprev[1] -= vpdiff;
		else
			state.valprev[1] += vpdiff;

		/* Step 4 - Clamp previous value to 16 bits */
		if ( state.valprev[1] > 32767 )
			state.valprev[1] = 32767;
		else if ( state.valprev[1] < -32768 )
			state.valprev[1] = -32768;

		/* Step 5 - Assemble value, update index and step values */
		delta |= sign;

		state.index[1] += indexTable[delta];
		if ( state.index[1] < 0 ) state.index[1] = 0;
		if ( state.index[1] > 88 ) state.index[1] = 88;
		step = stepsizeTable[state.index[1]];

		/* Step 6 - Output value */
		*outdata++ = (delta & 0x0f) | outputbuffer;
	}

	return bytescoded;
}

// len = input buffer size in bytes
int adpcm_decode_stereo(unsigned char * indata, short * outdata, int len){
	int sign = 0;					/* Current adpcm sign bit */
	int delta = 0;					/* Current adpcm output value */
	int step[2] = {0, 0};			/* Stepsize */
	int valpred[2] = {0, 0};		/* Predicted value */
	int vpdiff = 0;					/* Current change to valpred */
	int index[2] = {0, 0};			/* Current step change index */
	int inputbuffer = 0;			/* place to keep next 4-bit value */
	int bytesdecoded = 0;

	if ( !indata || !outdata || len < 1 ) return 0;

	valpred[0] = state.valprev[0];
	valpred[1] = state.valprev[1];
	index[0] = state.index[0];
	index[1] = state.index[1];
	step[0] = stepsizeTable[index[0]];
	step[1] = stepsizeTable[index[1]];

	for ( bytesdecoded = 0 ; bytesdecoded < len; bytesdecoded++ ) {

		inputbuffer = *indata++;

		/* Left Channel */
		/* Step 1 - get the delta value */
		delta = (inputbuffer >> 4) & 0xf;

		/* Step 2 - Find new index value (for later) */
		index[0] += indexTable[delta];
		if ( index[0] < 0 ) index[0] = 0;
		if ( index[0] > 88 ) index[0] = 88;

		/* Step 3 - Separate sign and magnitude */
		sign = delta & 8;
		delta = delta & 7;

		/* Step 4 - Compute difference and new predicted value */
		/*
		** Computes 'vpdiff = (delta+0.5)*step/4', but see comment
		** in adpcm_coder.
		*/
		vpdiff = step[0] >> 3;
		if (delta & 4 ) vpdiff += step[0];
		if (delta & 2 ) vpdiff += step[0]>>1;
		if (delta & 1 ) vpdiff += step[0]>>2;

		if ( sign ) valpred[0] -= vpdiff;
		else valpred[0] += vpdiff;

		/* Step 5 - clamp output value */
		if ( valpred[0] > 32767 ) valpred[0] = 32767;
		else if ( valpred[0] < -32768 ) valpred[0] = -32768;

		/* Step 6 - Update step value */
		step[0] = stepsizeTable[index[0]];

		/* Step 7 - Output value */
		*outdata++ = valpred[0];

		/* Right Channel */
		/* Step 1 - get the delta value */
		delta = inputbuffer & 0xf;

		/* Step 2 - Find new index value (for later) */
		index[1] += indexTable[delta];
		if ( index[1] < 0 ) index[1] = 0;
		if ( index[1] > 88 ) index[1] = 88;

		/* Step 3 - Separate sign and magnitude */
		sign = delta & 8;
		delta = delta & 7;

		/* Step 4 - Compute difference and new predicted value */
		/*
		** Computes 'vpdiff = (delta+0.5)*step/4', but see comment
		** in adpcm_coder.
		*/
		vpdiff = step[1] >> 3;
		if ( delta & 4 ) vpdiff += step[1];
		if ( delta & 2 ) vpdiff += step[1]>>1;
		if ( delta & 1 ) vpdiff += step[1]>>2;

		if ( sign ) valpred[1] -= vpdiff;
		else valpred[1] += vpdiff;

		/* Step 5 - clamp output value */
		if ( valpred[1] > 32767 ) valpred[1] = 32767;
		else if ( valpred[1] < -32768 ) valpred[1] = -32768;

		/* Step 6 - Update step value */
		step[1] = stepsizeTable[index[1]];

		/* Step 7 - Output value */
		*outdata++ = valpred[1];
	}

	state.valprev[0] = valpred[0];
	state.valprev[1] = valpred[1];
	state.index[0] = index[0];
	state.index[1] = index[1];

	return bytesdecoded * 4;
}

int adpcm_encode(short * indata, unsigned char * outdata, int len, int channels){
	if ( channels == 2 ){
		return adpcm_encode_stereo(indata, outdata, len);
	}
	return adpcm_encode_mono(indata, outdata, len);
}

int adpcm_decode(unsigned char * indata, short * outdata, int len, int channels){
	if ( channels == 2 ){
		return adpcm_decode_stereo(indata, outdata, len);
	}
	return adpcm_decode_mono(indata, outdata, len);
}

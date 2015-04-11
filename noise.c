#include "../include/portaudio.h"
#include "./memory-check.h"
#include "fft.h"
#include "hc.h"
#include <fftw3.h>
#include <stdlib.h>
#include <unistd.h>
#include <stdio.h>
#include <sndfile.h>
#include <sys/types.h>
#include <dirent.h>
#include <sys/stat.h>
#define FRAMES (1024)
#define CHANNELS (2)
#define SAMPLE_RATE (8000)
#define CHECK_OVERFLOW  (0)
int read_write_streams(void);
float* output_file(void);
typedef struct 
{
	float* data;
	int cursor;
	int num_frames;
} data;
static int output_callback(const void *inputBuffer, void *outputBuffer,
                          unsigned long framesPerBuffer,
                          const PaStreamCallbackTimeInfo* timeInfo,
                          PaStreamCallbackFlags statusFlags,
                          void *userData ) 
{ 
  	/* Intialization */
   float* out        = (float*)outputBuffer;
   data* data_struct = (data*) userData;
   int i;
  /* Intialization */
  for (i = 0; i < framesPerBuffer; i++)
  {
    //*memcpy(void *dest, const void *src, size_t n)

    if(data_struct->cursor < data_struct->num_frames*2)
    {
     	*out++ = data_struct->data[data_struct->cursor++];
     	*out++ = data_struct->data[data_struct->cursor++];

      //printf("%d\n",data_struct->cursor);
     	
    }
    else
    {
    	// pick in the 3/4 of the frames so cuts and randomize the location to not make it seem repetative  
    	data_struct->cursor = 3*data_struct->num_frames/2+rand()%2000-1000; 
    }

  }
  
  return  paContinue ;
  }

int main(void)
{

	read_write_streams();


	return 0; 

}
void inputsignal(fftw_complex* signal,float* Record, int numsamples) {
  int k;  
  for (k = 0; k < numsamples; k++) {
        
        signal[k][0] = Record[k];

        signal[k][1] = 0;
  		//	printf("%f \n",signal[k][0]);
	}
}



int read_write_streams(void)
{
	/* Declaration */ 
	PaStreamParameters input, output; 
	PaStream *stream_input,  *stream_output; 
	PaError error_input,error_output;
	float *recordsamples,*powerspec; //is recordsamples stereo if yes how is it formatted we need to average the channels in inputsignal!!!!!
  fftw_complex *in, *out;
  fftw_plan plan;
  float den;
  int i,totalframes,numsamples,numbytes; 
	data* struct_data;
	/* Declaration */
	
	struct_data = (data*) malloc(sizeof(data));

	/* Read the wav */
	struct_data->data = output_file();
	/* Read the wav */

	/* Intialization */
	totalframes   = 5*SAMPLE_RATE;
	numsamples    = CHANNELS*totalframes;
	numbytes  	  = numsamples * sizeof(float);
	recordsamples = (float*) malloc(numbytes);
  CHECK_MALLOC(recordsamples,"read_write_streams");
	powerspec = (float*) malloc(numbytes); //malloc too much memory but i think that cool we will figure it out
  in = (fftw_complex*) fftw_malloc(sizeof(fftw_complex) * numsamples);
	out = (fftw_complex*) fftw_malloc(sizeof(fftw_complex) * numsamples);
  CHECK_MALLOC(in,"read_write_streams");
  CHECK_MALLOC(out,"read_write_streams");
  plan = fftw_plan_dft_1d(numsamples, in, out, FFTW_FORWARD, FFTW_MEASURE);
  struct_data->cursor = 0;
	struct_data->num_frames = 441000;
  for (i = 0; i < numsamples; i++)
	{
		recordsamples[i] = 0;
    powerspec[i] = 0;
	}
	

	/* Port Audio Intialization */
	error_input = Pa_Initialize();
	if (error_input != paNoError)
	{
		goto error; 
	}
	error_output = Pa_Initialize();
	if (error_output != paNoError)
	{
		goto error; 
	}
	input.device = Pa_GetDefaultInputDevice();
	if (input.device == paNoDevice)
	{
		fprintf(stderr,"Error: No default input device.\n");
		goto error;
	}
	output.device = Pa_GetDefaultOutputDevice();
	if (output.device == paNoDevice)
	{
		fprintf(stderr,"Error: No default output device.\n");
      	goto error;
	}
	input.channelCount              =  CHANNELS;
	input.sampleFormat              =  paFloat32;
	input.suggestedLatency          =  Pa_GetDeviceInfo( input.device )->defaultLowInputLatency;
	input.hostApiSpecificStreamInfo =  NULL;

	output.channelCount              =  CHANNELS;
	output.sampleFormat              =  paFloat32;
	output.suggestedLatency          =  Pa_GetDeviceInfo( output.device )->defaultLowInputLatency;
	output.hostApiSpecificStreamInfo =  NULL;

	error_input = Pa_OpenStream( 
			  &stream_input,
              &input,
              NULL,                  /* &outputParameters, */
              SAMPLE_RATE,
              FRAMES,
              paClipOff,      /* we won't output out of range samples so don't bother clipping them */
              NULL, /* no callback, use blocking API */
              NULL);
	if (error_input !=paNoError)
	{
		goto error; 
	}
	error_output = Pa_OpenStream(
			  	&stream_output,
              NULL, 
            	&output,
           		44100,
            	paFramesPerBufferUnspecified,
            	paClipOff,      
            	output_callback, 
            	struct_data);
	if (error_output != paNoError)
	{
		goto error; 
	}
	error_input  = Pa_StartStream(stream_input);
	error_output = Pa_StartStream(stream_output);

	/* Intialization */

	/* Read and Write */
   while(1)
   {
   	/****************************************************************/
    // Okay so i am using fft.c for the power_spectrum_fftw function which uses
    // hc.c for the hc_2_amp function
    // I think the problem has to do with the fftw plan or something see if you can figure it out
    /****************************************************************/
      //error_output = Pa_WriteStream(stream_output,outputsamples, 441000);
      if( error_output  != paNoError && 0) goto error_o;
      //printf("HEREReading\n");
      error_input = Pa_ReadStream(stream_input,recordsamples, FRAMES);
      if(error_input != paNoError && 0) goto error;
      //do FFT PROCESSING
      inputsignal(in, recordsamples, numsamples);
      power_spectrum_fftw(numsamples,in,out,powerspec,den,4, plan);
      //print data
      for(i=0; i<numsamples/2; i++){
         printf("%f\n",powerspec[i]);
      } 
   }
   free(recordsamples);
   free(in);
   free(out);
error:

    Pa_Terminate();
    fprintf( stderr, "An error occured while using the portaudio stream_input\n" );
    fprintf( stderr, "Error number: %d\n", error_input );
    fprintf( stderr, "Error message: %s\n", Pa_GetErrorText( error_input) );

error_o:

    Pa_Terminate();
    fprintf( stderr, "An error occured while using the portaudio stream_input\n" );
    fprintf( stderr, "Error number: %d\n", error_output );
    fprintf( stderr, "Error message: %s\n", Pa_GetErrorText( error_output) );
    return 0;
}
float* output_file(void)
{
	/* Declaration */
    SNDFILE *sf;
    SF_INFO info;
    int num, num_items,f,sr,c;
    float *buf;
    char* path_name; 
    /* Declaration */

    /* Intialization */
    path_name = "shortSum.wav";
    sf = sf_open(path_name,SFM_READ,&info);
    if (sf == NULL)
    {
    	printf("Failed to open the file.\n");
        exit(-1);
    }
    f = info.frames;
    sr = info.samplerate;
    c = info.channels;
    printf("frames=%d\n",f);
    printf("samplerate=%d\n",sr);
    printf("channels=%d\n",c);
    num_items = f*c;
    printf("num_items=%d\n",num_items);
    /* Intialization */

    /* Reading the file */
	  buf = (float *) malloc(num_items*sizeof(float));
    num = sf_read_float(sf,buf,num_items);
    sf_close(sf);    
    //exit(-1);
    return buf;
}
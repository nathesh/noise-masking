#include "../include/portaudio.h"
#include <stdlib.h>
#include <unistd.h>
#include <stdio.h>
#include <sndfile.h>
#include <sys/types.h>
#include <dirent.h>
#include <sys/stat.h>
#include <string.h>

typedef struct 
{
  float* data;
  int cursor;
  int num_frames;
  int channels; 
  // maybe add the number of files thus int files; 
  // need to change num_frames if it is different for each frame
  // need to add the 10 bands 
} data;

#define FRAMES (1024)
#define CHANNELS (2)
#define SAMPLE_RATE (8000)
#define CHECK_OVERFLOW  (0)
int read_write_streams(void);
data* output_file(void);

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
    if(data_struct->cursor < data_struct->num_frames*data_struct->channels)
    {
      *out++ = data_struct->data[data_struct->cursor++];
      *out++ = data_struct->data[data_struct->cursor++];   
    }  
    else
    {
    	// pick in the 3/4 of the frames so cuts and randomize the location to not make it seem repetative  
       
    	 data_struct->cursor = 0;  
       return  paComplete;
    }

  }
  return  paContinue;
  }

int main(void)
{

	read_write_streams();


	return 0; 

}

int read_write_streams(void)
{
	/* Declaration */ 
	PaStreamParameters input, output; 
	PaStream *stream_input,  *stream_output; 
	PaError error_input,error_output;
	float *recordsamples,summation;
	int i,totalframes,numsamples,numbytes,y; 
	data* struct_data;
	/* Declaration */
	
	struct_data = (data*) malloc(sizeof(data));

	/* Read the wav */
	struct_data = output_file();

  /* Read the wav */

	/* Intialization */
	totalframes   = 5*SAMPLE_RATE;
	numsamples    = CHANNELS*totalframes;
	numbytes  	  = numsamples * sizeof(float);
	recordsamples = (float*) malloc(numbytes);
	for (i = 0; i < numsamples; i++)
	{
		recordsamples[i] = 0;
		
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
   	  //printf("HERE!\n");
      //error_output = Pa_WriteStream(stream_output,outputsamples, 441000);
      if( error_output  != paNoError && 0) goto error_o;
      //printf("HEREReading\n");
      if(Pa_IsStreamActive(stream_output)==0)
      {
        Pa_StopStream(stream_output);
        for(i = 0;i<struct_data->num_frames*2;i++) // is accessing num_frames bad?
        {
          summation = 0;
          for(y = 0; y<11;y++)
          {
            summation += struct_data->data[y*struct_data->num_frames*2+i];
          }
          struct_data->data[i] = summation/11;
        }
        error_output = Pa_StartStream(stream_output);
        printf("%s\n","Restarting Stream");
      }

      
      error_input = Pa_ReadStream(stream_input,recordsamples, FRAMES);
      if(error_input != paNoError && 0) goto error;

   }
   free(recordsamples);
	/* Read and Write */
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
data* output_file(void)
{
	  /* Declaration */
    SNDFILE *sf;
    SF_INFO info;
    int num, num_items,f,sr,c,files,x,counter;
    float *buf,*combination;
    char *path_name,buff[128]; 
    struct dirent **namelist;
    data* data_struct; 
    /* Declaration */

    /* Intialization */
    counter = 0;
    path_name = "data/";
    files = 0; 
    num = scandir(path_name, &namelist, 0, versionsort);
    combination = (float*) malloc(sizeof(float)*819144*11);
    data_struct = (data*) malloc(sizeof(data));
    data_struct->cursor = 0; 
    /* Intialization */

    /* Reading the file */
    if(num <0)
    {
      perror("Scandir");
    }
    else
    {
      while(files<num)
      {
        if(strcmp(namelist[files]->d_name,".") != 0 && strcmp(namelist[files]->d_name,".."))
        {
          printf("%d, %s\n",files,namelist[files]->d_name);
          strcpy(buff,path_name);
          strcat(buff,namelist[files]->d_name);
          sf = sf_open(buff,SFM_READ,&info);
          if(sf == NULL) 
          {
            printf("%s\n","ERROR OPENING THE FILE" );
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
          data_struct->num_frames = f; // Assuming the frames are the same
          data_struct->channels = c; 
          buf = (float *) malloc(num_items*sizeof(float)); // Need to free this buf
          sf_read_float(sf,buf,num_items);
          sf_close(sf);
          for(x=0;x<num_items;x++)
          {
            combination[counter*num_items+x] = buf[x];

          }
          free(buf);
          counter++;

        }
        free(namelist[files]);
        files++;
      }
      free(namelist);
    }
    
    
    sf_close(sf);   
    data_struct->data  = combination; 
    return data_struct;
}

/*  TO DO 
1. return the data struct after reading all the files 
2. Do the processing in the return callback (multipling and summing the bands callback)
3. Logically this should changing the one of the pointers in the struct changes the values in the callback

*/
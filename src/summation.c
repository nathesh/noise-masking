#include "portaudio.h"
#include <stdlib.h>
#include <unistd.h>
#include <stdio.h>
#include <sndfile.h>
#include <sys/types.h>
#include <dirent.h>
#include <sys/stat.h>
#include <string.h>
#include "./memory-check.h"
#include "fft.h"
#include "hc.h"
#include <fftw3.h>
#include <math.h>

typedef struct 
{
  float* data,*noise;
  int cursor;
  int num_frames;
  int channels; 
  // maybe add the number of files thus int files; 
  // need to change num_frames if it is different for each frame
  // need to add the 10 bands 
} data;

#define FRAMES (256)
#define CHANNELS (2)
#define SAMPLE_RATE (8000)
#define CHECK_OVERFLOW  (0)
#define LINEAR (1) //0 for octave NUM_BANDS
#define NUM_BANDS (10)
#define max(a,b) \
   ({ __typeof__ (a) _a = (a); \
       __typeof__ (b) _b = (b); \
     _a > _b ? _a : _b; })

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
       //printf("%f\n",out[i]);
      *out++ = data_struct->data[data_struct->cursor++];   
       //printf("%f\n",out[i+1]);
    }  
    else
    {
    	// pick in the 3/4 of the frames so cuts and randomize the location to not make it seem repetative  
       
    	 data_struct->cursor = 3*data_struct->num_frames/2+rand()%2000-1000;  
       *out++ = data_struct->data[data_struct->cursor++];
       //printf("%f\n",out[i]);
       *out++ = data_struct->data[data_struct->cursor++];   
       //printf("%f\n",out[i+1]);
       //return  paComplete;
    }

  }
  return  paContinue;
  }

  //converts microphone data to mono and converts to fftw_complex type 
void inputsignal(fftw_complex* signal,float* Record, int numsamples) {
  int k; 
  float temp;  
  FILE *inputsignal; 
  inputsignal = fopen("../streams/inputsignal.txt","w");
 // int i; //counter
 //i dont know if this will work if numsamples is odd....
  for (k = 0; k < numsamples; k++) {
       if(CHANNELS == 2){
         if(k%2 == 0){
           signal[k/2][0] = (Record[k]+Record[k+1])/2;
        //   printf("%f,\n",Record[k]);
           signal[k/2][1] = 0;
         }
       }
       else{
         signal[k][0] = Record[k];
         signal[k][1] = 0;
       }
       //mono microphone input data 
       temp = signal[k][0];
       if(inputsignal == NULL)
       {
        printf("%s\n", "BABY JESUSD WE HAVE A");
       }
       fprintf(inputsignal,"%f\n",signal[k][0]);
  }
  fclose(inputsignal);
}

void A_compute_coeff(int n, float* A, float fres) {
  int i;
  int freq, freq1; //or float???? lose precision....
  float Y1 = pow(12200,2);
  float Y2 = pow(20.6,2);
  float Y3 = pow(107.7,2);
  float Y4 = pow(737.9,2);
  for (i=0; i < n; i++) 
{      freq1 = fres*i;
       freq = pow(i * fres,2);
       A[i] =20*log10((Y1 *pow(freq,2))/ 
              ((Y1 + freq) * (Y2 + freq) *  sqrt((Y3 + freq) * (Y4 + freq))))+2;
    }
}


void compute_band_weights(int n, float* p, float fres,float* out, float* bands)
{
  //in is PSD data 
  //out is array of 10 values (NUM_BANDS) 
  //map values from 0-1 
  int i,k;
  float maxVal,sum,avg,threshold;
  maxVal = -200;
  i   = 0;
  avg = 0;
  sum = 0;
  threshold = .0001;

//calculate running average in each band  
  for (k = 0; k < n/2; k++) {
      if ((bands[i] <= (k*fres)) && ((k*fres) <= bands[i+1])) {
         maxVal = max(maxVal,p[k]);
//         printf("%f\n",maxVal);
        // out[i] /= 2;
      } 
      else if ((LINEAR && ((k*fres)<bands[0])) || (!LINEAR && ((k*fres)<bands[0]))) {
        // if between -0 80 in linear bands ignor 
      }   
      else if (bands[i] > SAMPLE_RATE/2 ){
         break;
      }
      else {
          out[i] = maxVal;
  //       printf("%f\n",maxVal);
          avg += maxVal;
          maxVal = -200;
          i ++; 
      }
  }
  out[i] = maxVal;
//calculate normalized weights don't add bands that arent measured (ie weight = 1)
//maybe set weights = to avg of weights in bands that arent measured....
  for (i = 0; i < NUM_BANDS; i++) {
    if(out[i] > threshold){ 
      if (out[i] != .75){
          sum += out[i];
      }
  }
}
  for (i = 0; i < NUM_BANDS; i++) {
    if(out[i] > threshold){ 
      if (out[i] != .75){
         out[i] /= sum;   
    }
    
    /* if ((out[i] != 1)&&(out[i]<0)){
         out[i] /= sum; 
      }
      else if((out[i] != 1)&&(out[i]>0)){
         out[i] /= -1*sum;
      }*/
    }
     out[i] += .75;
  }

//bands that are not measured set equal to avg of weights....
  for (i = 0; i < NUM_BANDS; i++) {
    if (out[i] >1) {
       out[i] = 1;
    }  
 //    printf("bands:%f weights:%f\n",bands[i],out[i]);
  }
  
}

void A_weighting(int n, float* weights, float* in)
{
  int i;
  for(i=0; i<n/2; i++){
    in[i] = 10*log10(in[i]);
  //  printf("%f\n",weights[i]);
    in[i] = weights[i] + in[i];
  }
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
	float den, fres, *recordsamples,*powerspec, *A, *weights, *bands, summation;
	int i,totalframes,numsamples,numbytes,y,counter; //y might cause problems 
	fftw_complex *in, *out;
  fftw_plan plan;data* struct_data;
  FILE *powerspec_file; 
  counter = 0; 
	/* Declaration */
	
	struct_data = (data*) malloc(sizeof(data));

	/* Read the wav */
	struct_data = output_file();
  for(i = 0;i<struct_data->num_frames*2;i++) // is accessing num_frames bad?
  {
    summation = 0;
    for(y = 0; y <11;y++)
    {
      summation += struct_data->noise[y*struct_data->num_frames*2+i];
    }
    struct_data->data[i] = summation;
  }
  /* Read the wav */

	/* Intialization */
  totalframes   = CHANNELS*FRAMES;//5*SAMPLE_RATE; why????
  numsamples    = FRAMES; //why totalframes is 5*sample rate change to totalframes if wrong 
  fres          = (float) SAMPLE_RATE/FRAMES;
  numbytes      = numsamples * sizeof(float);
  recordsamples = (float*) malloc(numbytes);
  CHECK_MALLOC(recordsamples,"read_write_streams");
  powerspec = (float*) malloc(numbytes); //malloc too much memory but i think that cool we will figure it out
  CHECK_MALLOC(powerspec,"read_write_streams"); 
  A = (float*) malloc(numbytes);
  CHECK_MALLOC(A,"read_write_streams");
  weights = (float*)malloc(sizeof(float)*NUM_BANDS); //final computed band weights
  CHECK_MALLOC(weights,"read_write_streams");
  bands = (float*)malloc(sizeof(float)*NUM_BANDS);  //bands specified to compute band weights
  CHECK_MALLOC(bands,"read_write_streams");
  in = (fftw_complex*) fftw_malloc(sizeof(fftw_complex) * numsamples);
  out = (fftw_complex*) fftw_malloc(sizeof(fftw_complex) * numsamples);
  CHECK_MALLOC(in,"read_write_streams");
  CHECK_MALLOC(out,"read_write_streams");
  plan = fftw_plan_dft_1d(numsamples, in, out, FFTW_FORWARD, FFTW_MEASURE);
  struct_data->cursor = 0;
  //struct_data->num_frames = 441000;
  for (i = 0; i < numsamples; i++){
    recordsamples[i] = 0;
    powerspec[i] = 0; //should be half the size of the recorded samples
    A[i] = 0;
  }
  for (i = 0; i <= NUM_BANDS; i++){
    weights[i] = .75;  //init weights to 1 equal volume 
  }
  if (LINEAR == 1){
     bands[0] = 80;
     for (i = 1; i <= NUM_BANDS; i++) {
         bands[i] = i*(SAMPLE_RATE/2)/NUM_BANDS;
     }
  }
  else{
     bands[0] = 60;
     bands[1] = 125;
     bands[2] = 250;
     bands[3] = 500;
     bands[4] = 1000;
     bands[5] = 2000;
     bands[6] = 4000;
     bands[7] = 8000;
     bands[8] = 11000;
     bands[9] = 22000; 
 }

  A_compute_coeff(numsamples/2,A,fres);

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
   //sleep(100); 
   float randnum;
   randnum = 1; 
   //powerspec_file = fopen("../streams/powerspec.txt","w");
   while(1)
   {
   	  //printf("HERE!\n");
      //error_output = Pa_WriteStream(stream_output,outputsamples, 441000);
      powerspec_file = fopen("../streams/powerspec.txt","w");
      counter++;
      if( error_output  != paNoError && 0) goto error_o;
      error_input = Pa_ReadStream(stream_input,recordsamples, FRAMES);
      if(error_input != paNoError && 0) goto error;
      
      
      inputsignal(in, recordsamples, CHANNELS*numsamples); //converts to fftw_complex and averages stereo to mono
      weighted_power_spectrum_fftw(numsamples,in,out,powerspec,den,4, plan);

       //do FFT PROCESSING
      compute_band_weights(numsamples,powerspec,fres,weights,bands);
      A_weighting(numsamples,A,powerspec); 
      for (i = 0; i < numsamples/2; i++){
        fprintf(powerspec_file,"%f,%f\n",fres*i,powerspec[i]);  //fres*i = xaxis power= yaxis
      }  
      //sleep(156);
      //sleep(10);
          
          //printf("%f\n",weights[0]);
          for(i = 0;i<struct_data->num_frames*2;i++) // is accessing num_frames bad?
            {
              summation = 0;
              for(y = 0; y <11;y++)
              {
                summation += weights[y]*.7*struct_data->data[y*struct_data->num_frames*2+i];
              }
              struct_data->data[i] = summation;
            }
          //printf("y=%d,i=%d,numframe=%d,maxnum=%d\n",y,i,struct_data->num_frames,y*struct_data->num_frames*2+i);
      //print data
      
      fclose(powerspec_file);
   }
   fclose(powerspec_file);
   free(recordsamples);
   free(in);
   free(out);
  //clean up other stuff fft and other malloced items

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
    int num, num_items,f,sr,c,files,x,counter,i;
    float *buf,*combination, *noise_1;
    char *path_name,buff[128]; 
    struct dirent **namelist;
    data* data_struct; 
    /* Declaration */

    /* Intialization */
    counter = 0;
    path_name = "../data/";
    files = 0; 
    num = scandir(path_name, &namelist, 0, alphasort);
    combination = (float*) malloc(sizeof(float)*819144*11);
    noise_1 = (float*) malloc(sizeof(float)*819144*11);
    data_struct = (data*) malloc(sizeof(data));
    data_struct->cursor = 0; 
    /* Intialization */
    for(i=0;i<819144*11;i++)
    {
      noise_1[i] = 0;
      combination[i] = 0;
    }
    /* Reading the file */
    if(num <0)
    {
      perror("Scandir");
    }
    else
    {
      while(files<num)
      {
        printf("%d\n",counter);
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
          //printf("counter=%d\n",counter);
          //printf("Max=%d\n",counter*num_items+num_items);
          for(x=0;x<num_items;x++)
          {
            combination[counter*num_items+x] = buf[x];
            noise_1[counter*num_items+x] = buf[x];
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
    data_struct->noise = noise_1;
    return data_struct;
}

/*  TO DO 
1. Blocking writing with signed long Pa_GetStreamWriteAvailable ( PaStream *  stream  ) 
2. Implement above with constant updates; check if infinite 


*/
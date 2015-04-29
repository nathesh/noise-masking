#include "portaudio.h"
#include <stdbool.h>
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

#define FRAMES (256)
#define CHANNELS (2)
#define SAMPLE_RATE (8000)
#define CHECK_OVERFLOW  (0)
#define max(a,b) \
   ({ __typeof__ (a) _a = (a); \
       __typeof__ (b) _b = (b); \
     _a > _b ? _a : _b; })

/* maybe add the number of files thus int files; 
   need to add the 10 bands */
typedef struct 
{ 
  float* data,*noise;
  int cursor;
  int num_frames; /* need to change num_frames if it is different for each frame */
  int channels; 
} data;


/* Function Prototypes */
int read_write_streams(char* bandSpacing, char* maskingNoise, char* maskingType);
data* output_file(int numBands, bool linear, bool rain);
void A_compute_coeff(int n, float* A, float fres);
static int output_callback(const void *inputBuffer, 
                           void *outputBuffer,
                           unsigned long framesPerBuffer,
                           const PaStreamCallbackTimeInfo* timeInfo,
                           PaStreamCallbackFlags statusFlags,
                           void *userData 
                           ) ;
void inputsignal(fftw_complex* signal, float* Record, int numsamples);
void compute_band_weights(int numBands, bool linear, int n, float* p, float fres,float* out, float* bands);

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

int main(int argc, char* argv[])
{ char *linear, *maskNoise, *bandSpacing, *maskType;
 
   if (argc != 4) /* Report error and exit */
    { printf("Error need 3 arguments");  return (-1); } 

    maskNoise     = argv[1];
    maskType      = argv[2];
    bandSpacing   = argv[3];
    read_write_streams(bandSpacing, maskNoise, maskType);

  return 0; 
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

/***************** read_write_streams *****************
 * inputs: char* bandSpacing
 *         char* maskNoise
 *         char* maskType
 * outputs: int (literal)
 Description:
  ...
 ****************************************************** */
int read_write_streams(char* bandSpacing, char* maskNoise, char* maskType)
{
/* DECLARATION */ 
  float den, fres, *recordsamples,*powerspec, *A, *weights, *bands, summation;
  int i, numBands, totalframes, numsamples, numbytes, y, counter; //y might cause problems 
  bool dynamic, rain, linear;

  PaStreamParameters input, output; 
  PaStream *stream_input,  *stream_output; 
  PaError error_input,error_output;
  fftw_complex *in, *out;
  fftw_plan plan;
  FILE *powerspec_file; 
  data* struct_data; 
  
/* INITIALIZE STATE VARIABLES */
  counter = 0;
  struct_data = (data*) malloc(sizeof(data));

  // Convert command line args to booleans 
  dynamic = !strcmp(maskType,"Dynamic")?   1:0;
  rain    = !strcmp(maskNoise, "Rain")?    1:0;
  linear  = !strcmp(bandSpacing, "linear")? 1:0;
  if(linear) numBands = 10;
  else       numBands = 7;

/* READ THE .WAV */
  struct_data = output_file(numBands,linear,rain);
  for(i=0; i<(2*struct_data->num_frames); i++) // is accessing num_frames bad?
  {
    summation = 0;
    for(y = 0; y <= numBands; y++)
      summation += struct_data->noise[2*y*struct_data->num_frames+i];
    
    struct_data->data[i] = summation;
  }

/* ALLOCATE MEMORY FOR DATA STRUCTURES */
  totalframes   = CHANNELS*FRAMES;//5*SAMPLE_RATE; why????
  numsamples    = FRAMES; //why totalframes is 5*sample rate change to totalframes if wrong 
  fres          = (float) SAMPLE_RATE/FRAMES;
  numbytes      = numsamples * sizeof(float);
  recordsamples = (float*) malloc(numbytes);
  powerspec     = (float*) malloc(numbytes); //malloc too much memory but i think that cool we will figure it out
  A             = (float*) malloc(numbytes);
  weights       = (float*)malloc(sizeof(float)*numBands); //final computed band weights
  bands         = (float*)malloc(sizeof(float)*numBands);  //bands specified to compute band weights
  in            = (fftw_complex*) fftw_malloc(sizeof(fftw_complex) * numsamples);
  out           = (fftw_complex*) fftw_malloc(sizeof(fftw_complex) * numsamples);
  CHECK_MALLOC(recordsamples,"read_write_streams");
  CHECK_MALLOC(powerspec,"read_write_streams"); 
  CHECK_MALLOC(weights,"read_write_streams");
  CHECK_MALLOC(bands,"read_write_streams");
  CHECK_MALLOC(in,"read_write_streams");
  CHECK_MALLOC(out,"read_write_streams");
  CHECK_MALLOC(A,"read_write_streams");
  
  
  plan                = fftw_plan_dft_1d(numsamples, in, out, FFTW_FORWARD, FFTW_MEASURE);
  struct_data->cursor = 0;


  //struct_data->num_frames = 441000;
  for (i = 0; i < numsamples; i++)
    recordsamples[i] = powerspec[i] = A[i] = 0;
  for (i = 0; i <= numBands; i++)
    weights[i]      = .75;  //init weights to 1 equal volume 
  
  if (linear)
  { bands[0] = 80;
    for (i = 1; i <= numBands; i++)
      bands[i] = i*(SAMPLE_RATE/2)/numBands;
  }
  else
  {
     bands[0] = 60;
     bands[1] = 125;
     bands[2] = 250;
     bands[3] = 500;
     bands[4] = 1000;
     bands[5] = 2000;
     bands[6] = 4000;
     bands[7] = 8000;
 }

  A_compute_coeff(numsamples/2,A,fres);

/* PORT AUDIO INIT W/ ERROR CHECKING */
  if( paNoError != (error_input = Pa_Initialize()) ) goto error;
  if( paNoError != (error_output = Pa_Initialize()) ) goto error;
  if( paNoDevice == (input.device = Pa_GetDefaultInputDevice()))
    { fprintf(stderr, "Error: No default input device. \n");  goto error; }
  if( paNoDevice == (output.device = Pa_GetDefaultOutputDevice()))
    { fprintf(stderr, "Error: No default output device. \n");  goto error; }

 
  input.channelCount              =  CHANNELS;
  input.sampleFormat              =  paFloat32;
  input.suggestedLatency          =  Pa_GetDeviceInfo( input.device )->defaultLowInputLatency;
  input.hostApiSpecificStreamInfo =  NULL;

  output.channelCount              =  CHANNELS;
  output.sampleFormat              =  paFloat32;
  output.suggestedLatency          =  Pa_GetDeviceInfo( output.device )->defaultLowInputLatency;
  output.hostApiSpecificStreamInfo =  NULL;

  // Open the port audio stream for input
  error_input = Pa_OpenStream( 
        &stream_input,
              &input,
              NULL,                  /* &outputParameters, */
              SAMPLE_RATE,
              FRAMES,
              paClipOff,      /* we won't output out of range samples so don't bother clipping them */
              NULL, /* no callback, use blocking API */
              NULL);
  if (error_input !=paNoError) goto error; 
  
  // Open the port audio stream for output
  error_output = Pa_OpenStream(
          &stream_output,
                NULL, 
              &output,
              44100,
              paFramesPerBufferUnspecified,
              paClipOff,      
              output_callback, 
              struct_data);
  if (error_output != paNoError) goto error; 
  



  error_input  = Pa_StartStream(stream_input);
  error_output = Pa_StartStream(stream_output);

  /* Intialization */

  /* Read and Write */
   while(1)
   {
      powerspec_file = fopen("../streams/powerspec.txt","w");
      if( error_output  != paNoError && 0) goto error_o;
      error_input = Pa_ReadStream(stream_input,recordsamples, FRAMES);
      if(error_input != paNoError && 0) goto error;


      /* FFT PROCESSING */
      inputsignal(in, recordsamples, CHANNELS*numsamples); //converts to fftw_complex and averages stereo to mono
      weighted_power_spectrum_fftw(numsamples,in,out,powerspec,A,den,4, plan);

      //compute bands if dynamic masking
      if (dynamic)
        compute_band_weights(numBands,linear,numsamples,powerspec,fres,weights,bands);
      

      for(i = 0;i<struct_data->num_frames*2;i++) // is accessing num_frames bad?
        {
          summation = 0;
          for(y = 0; y <= numBands;y++)
            summation +=  weights[y]*struct_data->data[y*struct_data->num_frames*2+i];
    
          struct_data->data[i] = summation;
        }
      fclose(powerspec_file);
   }

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



/***************** output_file *****************
 * inputs: int numBands 
 *         bool linear // linear or octave?
 *         bool rain  // white noise type
 * outputs: data* data_struct
  Description:
   ...
 *********************************************** */
data* output_file(int numBands, bool linear, bool rain)
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
    if (rain)
       if(linear) path_name = "../data/rain/linear/";
       else path_name = "../data/rain/octave/";
    else
       if(linear) path_name = "../data/creek/linear/";
       else path_name = "../data/creek/octave/";
    files = 0; 
    num = scandir(path_name, &namelist, 0, alphasort);
    combination = (float*) malloc(sizeof(float)*819144*(numBands+1));
    noise_1 = (float*) malloc(sizeof(float)*819144*(numBands+1));
    data_struct = (data*) malloc(sizeof(data));
    data_struct->cursor = 0; 
    /* Intialization */
    for(i=0;i<819144*(numBands+1);i++)
    {
      noise_1[i] = 0;
      combination[i] = 0;
    }
    /* Reading the file */
    if(num <0)
      perror("Scandir");
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
          if(sf == NULL) { printf("%s\n","ERROR OPENING THE FILE" ); exit(-1); }

          printf("frames=%d \n", f = info.frames);
          printf("samplerate=%d \n", sr = info.samplerate);
          printf("channels=%d \n", c = info.channels);
          printf("num_items=%d \n", num_items = f*c);

          data_struct->num_frames = f; // Assuming the frames are the same
          data_struct->channels = c; 
          buf = (float *) malloc(num_items*sizeof(float)); // Need to free this buf
          sf_read_float(sf,buf,num_items);
          sf_close(sf);

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


/***************** A_compute_coeff *****************
 * inputs: int n 
 *         float *A
 *         float fres
 * outputs: void
  Description:
   ...
 *************************************************** */
void A_compute_coeff(int n, float* A, float fres) {
  int i;
  int freq, freq1; //or float???? lose precision....
  static float Y1 = (12200*12200);
  static float Y2 = (20.6*20.6);
  static float Y3 = (107.7*107.7);
  static float Y4 = (737.9*737.9);
  for (i=0; i < n; i++) 
  {    //freq1 = fres*i;
       freq  = i * fres * i * fres;
       A[i]  = 20*log10((Y1 * freq * freq)/ 
                    ((Y1 + freq) * (Y2 + freq) *  sqrt((Y3 + freq) * (Y4 + freq))))+2;
  }
}


/***************** output_callback *****************
 * inputs: const void *inputBuffer 
 *         void *outputBuffer
 *         unsigned long framesPerBuffer
 *         const PaStreamCallbackTimeInfo* timeInfo
 *         PaStreamCallbackFlags statusFlags
 *         void *userData
 * outputs: static int paContinue
  Description:
   ...
 *************************************************** */
static int output_callback(const void *inputBuffer, 
                           void *outputBuffer,
                           unsigned long framesPerBuffer,
                           const PaStreamCallbackTimeInfo* timeInfo,
                           PaStreamCallbackFlags statusFlags,
                           void *userData ) 
{ 
    /* Intialization */
  int i, sat, curs;
  float* out        = (float*)outputBuffer;
   

  data* data_struct = (data*) userData;
  sat = data_struct->num_frames*data_struct->channels;
  curs = 3*data_struct->num_frames/2-1000;

  for (i = 0; i < framesPerBuffer; i++)
  {
    if(data_struct->cursor < sat)
    {
      *out++ = data_struct->data[data_struct->cursor++];
      *out++ = data_struct->data[data_struct->cursor++];   
    }  
    else
    {
      // pick in the 3/4 of the frames so cuts and randomize the location to not make it seem repetative  
       data_struct->cursor = curs + rand() % 2000;  
       *out++ = data_struct->data[data_struct->cursor++];
       *out++ = data_struct->data[data_struct->cursor++];   
    }

  }

 return  paContinue;
}





/***************** inputsignal *****************
 * inputs: fftw_complex* signal
 *         float* Record
 *         int numsamples
 * outputs: void
  Description:
   converts microphone data to mono and converts 
   to fftw_complex type
 *************************************************** */ 
void inputsignal(fftw_complex* signal,float* Record, int numsamples) {
  int k;  
 // int i; //counter
 //i dont know if this will work if numsamples is odd....
  for (k = 0; k < numsamples; k++) 
  {
       if( (k % 2 == 0) && CHANNELS == 2) 
       {
           signal[k/2][0] = (Record[k]+Record[k+1])/2;
           signal[k/2][1] = 0;
       }
       else
       {
           signal[k][0] = Record[k];
           signal[k][1] = 0;
       } 
  }
}


/***************** compute_band_weights *****************
 * inputs: int numBands
 *         bool linear
 *         int n
 *         float* p
 *         float fres
 *         float* out
 *         float* bands
 * outputs: void
  Description:
   ...
 ********************************************************* */
void compute_band_weights(int numBands, bool linear, int n, float* p, float fres,float* out, float* bands)
{
  //in is PSD data 
  //out is array of 10 values (numBands) 
  //map values from 0-1 
  int i, k;
  float maxVal, sum, avg, threshold;

  maxVal    = -200;
  i         = 0;
  avg       = 0;
  sum       = 0;
  threshold = .0001;

//calculate running average in each band  
  for (k = 0; k < n/2; k++) 
  {
      if ((bands[i] <= (k*fres)) && ((k*fres) <= bands[i+1])) 
      {
         maxVal = max(maxVal,p[k]);
      }    
      else if (bands[i] > SAMPLE_RATE/2 ){
         break;
      }
      else {
          out[i] = maxVal;
          avg += maxVal;
          maxVal = -200;
          i ++; 
      }
  }
  out[i] = maxVal;
//calculate normalized weights don't add bands that arent measured (ie weight = 1)
//maybe set weights = to avg of weights in bands that arent measured....
  for (i = 0; i < numBands; i++) 
  {
    if(out[i] > threshold && out[i] != .75)
      sum += out[i];
  
  }
  for (i = 0; i < numBands; i++)
  {
    if(out[i] > threshold && out[i] != .75)
      out[i] /= sum;   
    
    out[i] += .75;

    if (out[i] >1) 
       out[i] = 1;
  }

//bands that are not measured set equal to avg of weights....
  /*
  for (i = 0; i < numBands; i++) 
  {
     printf("bands:%f weights:%f\n",bands[i],out[i]);
  }
  */

  return;
}




/*  TO DO 
1. Blocking writing with signed long Pa_GetStreamWriteAvailable ( PaStream *  stream  ) 
2. Implement above with constant updates; check if infinite 


*/
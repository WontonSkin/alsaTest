/*

This example reads standard from input and writes
to the default PCM device for 5 seconds of data.

gcc -o alsa_play alsa_play.c -L. -lasound
*/

/* Use the newer ALSA API */

#include <stdio.h>
#include <stdlib.h>
#include "alsa/asoundlib.h"

#define ALSA_PCM_NEW_HW_PARAMS_API

int main(int argc, char *argv[])
{
    long loops;
    int rc;
    int size;
    snd_pcm_t *handle;
    snd_pcm_hw_params_t *params;
    unsigned int val;
    int dir;
    snd_pcm_uframes_t frames;
    snd_pcm_uframes_t tmp_frames;
    char *buffer = NULL;

    if (argc != 2)
    {
        printf("error: alsa_play [music name]. just support pcm.\n");
        exit(1);
    }

    /* Open PCM device for playback. */
    //rc = snd_pcm_open(&handle, "default", SND_PCM_STREAM_PLAYBACK, 0);
    rc = snd_pcm_open(&handle, "plughw:0,0", SND_PCM_STREAM_PLAYBACK, 0);
    if (rc < 0)
    {
        fprintf(stderr,"unable to open pcm device: %s\n", snd_strerror(rc));
        exit(1);
    }

    /* Allocate a hardware parameters object. */
    snd_pcm_hw_params_alloca(&params);

    /* Fill it in with default values. */
    snd_pcm_hw_params_any(handle, params);

    /* Set the desired hardware parameters. */

    /* Interleaved mode */
    snd_pcm_hw_params_set_access(handle, params, SND_PCM_ACCESS_RW_INTERLEAVED);

    /* Signed 16-bit little-endian format */
    snd_pcm_hw_params_set_format(handle, params, SND_PCM_FORMAT_S16_LE);

    /* Two channels (stereo) */
    snd_pcm_hw_params_set_channels(handle, params, 2);

    /* 44100 bits/second sampling rate (CD quality) */
    val = 44100;
    snd_pcm_hw_params_set_rate_near(handle, params, &val, &dir);

    /* Set period size to 32 frames. */
    frames = 256;
    snd_pcm_hw_params_set_period_size_near(handle, params, &frames, &dir);

    /* Write the parameters to the driver */
    rc = snd_pcm_hw_params(handle, params);
    if (rc < 0)
    {
        fprintf(stderr, "unable to set hw parameters: %s\n", snd_strerror(rc));
        exit(1);
    }

    /* Use a buffer large enough to hold one period */
    snd_pcm_hw_params_get_period_size(params, &frames, &dir);
    size = frames * 4; /* 2 bytes/sample, 2 channels */
    buffer = (char *) malloc(size);

    /* We want to loop for 5 seconds */
    snd_pcm_hw_params_get_period_time(params, &val, &dir);
    /* 5 seconds in microseconds divided by
    * period time */
    //loops = 5000000 / val;  //5S

    printf("frames:%d, size:%d val:%d.\n", frames, size, val);

    /* open audio file */
    printf("play song: %s \n", argv[1]);
    FILE *fp = fopen(argv[1], "rb");
    if(fp == NULL)
    {
        printf("fopen failed.\n");
        return 1;
    }

    /* get file size */
    fseek(fp, 0, SEEK_END);
    int song_size = ftell(fp);  
    printf("song size: %d\n", song_size); 
    fseek(fp, 0, SEEK_SET);

    //while (loops > 0) //循环录音 5 s 
    while (1)
    {
        //loops--;
        //fprintf(stderr, "loops:%d.\n", loops);
        tmp_frames = frames;
        rc = fread(buffer, 1, size, fp);
        if (rc == 0) //没有读取到数据 
        {
            fprintf(stderr, "end of file on input\n");
            break;
        } 
        else if (rc != size)//实际读取 的数据 小于 要读取的数据 
        {
            fprintf(stderr,"short read: read %d bytes\n", rc);
            tmp_frames = (rc >> 2);
        }

        rc = snd_pcm_writei(handle, buffer, tmp_frames);//写入声卡  （放音） 
        if (rc == -EPIPE) 
        {
            /* EPIPE means underrun */
            fprintf(stderr, "underrun occurred\n");
            snd_pcm_prepare(handle);
        } 
        else if (rc < 0)
        {
            fprintf(stderr,"error from writei: %s\n", snd_strerror(rc));
        }
        else if (rc != (int)tmp_frames)
        {
            fprintf(stderr,"short write, write %d frames\n", rc);
        }
    }

    snd_pcm_drain(handle);
    snd_pcm_close(handle);
    free(buffer);
    fclose(fp);

  return 0;
}

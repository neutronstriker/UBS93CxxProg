//in this project build options linker settings have been modified
//to link to libusb.a

#include"libusb\include\lusb0_usb.h"
#include <stdio.h>
#include <stdlib.h>
#include<stdint.h> //for typedefs of uint8_t and uint16_t etc
#include<ctype.h> //for tolower();

extern usb_dev_handle * usbOpenDevice(int VID, char * vendor, int PID, char * product);
extern void list_all_usb_devs();

void usage(char * filename)
{
    printf("\n\tUsage :\n");
    printf("\t%s read <address> : Read word from specified Address\n",filename);
    printf("\t%s read all       : Read the whole chip\n",filename);
    printf("\t%s read2f <file>  : Read data to a .bin file from chip\n",filename);
    printf("\t%s write <a> <w>  : Write a word; 'w' is word, 'a' is address \n",filename);
    printf("\t%s writef <file>  : Write Data from a .bin to chip\n",filename);
    printf("\t%s ON             : Turn On Test LED\n",filename);
    printf("\t%s OFF            : Turn Off Test LED\n",filename);

}

#define USB_LED_OFF 0
#define USB_LED_ON  1

#define READ_WORD	2
#define WRITE_WORD	3

#define READ_BYTE 	4
#define WRITE_BYTE	5

usb_dev_handle * handle = NULL;

uint16_t soft_clk_flag = 0;


int read_word(uint16_t *word,uint8_t address)
{
    //remember to use this function usb_dev_handle * variable must
    //be declared a global variable and don't use this fucntion without
    //opening the usb device
    if(handle == NULL)
    {
         fprintf(stderr,"\nError! Programmer Not Found\n");
         return -2;
    }

    char buffer[2];

    int flag;

    flag = usb_control_msg(handle, USB_TYPE_VENDOR|USB_RECIP_DEVICE
                           |USB_ENDPOINT_IN,READ_WORD,soft_clk_flag,address,buffer,sizeof(buffer),5000);

    *word = (uint8_t)buffer[0];
    *word = *word<<8;
    *word |= (uint8_t)buffer[1];

    return flag;


}

int write_word(uint16_t word,uint8_t address)
{
    //remember to use this function usb_dev_handle * variable must
    //be declared a global variable and don't use this fucntion without
    //opening the usb device
    if(handle == NULL)
    {
         fprintf(stderr,"\nError! Programmer Not Found\n");
         return -2;
    }

    int flag;

    char buffer[2];

    flag = usb_control_msg(handle,USB_TYPE_VENDOR|USB_RECIP_DEVICE
                           |USB_ENDPOINT_IN,WRITE_WORD,word,address,buffer,sizeof(buffer),5000);
    return flag;
}

int led(uint8_t state)
{
    if(state >= USB_LED_ON)
        state = USB_LED_ON;
    return usb_control_msg(handle,USB_TYPE_VENDOR|USB_RECIP_DEVICE|USB_ENDPOINT_IN,
                           state,0,0,0,0,5000);
}

int check_type()
{
    uint16_t w_SoftClk, wo_SoftClk;
    uint16_t sig_word = 0x5555;
    uint16_t data_wclk,data_woclk;

    //save original chip data
    soft_clk_flag = 0;
    read_word(&wo_SoftClk,0);//read word from address 0 without using SOFTCLK
    soft_clk_flag = 1;
    read_word(&w_SoftClk,0);//read word from address 0 using softclk

    //now lets check
    write_word(sig_word,0);//write signature word to address 0
    soft_clk_flag = 0;
    read_word(&data_woclk,0);
    soft_clk_flag = 1;
    read_word(&data_wclk,0);

    if(sig_word == data_wclk)
    {
        soft_clk_flag = 1;
        write_word(w_SoftClk,0);
        printf("This chip requires Soft CLk\n");
    }
    else if(sig_word == data_woclk)
    {
        soft_clk_flag = 0;
        write_word(wo_SoftClk,0);
        printf("This chip does not require Soft CLk\n");
    }
    else
    {
        fprintf(stderr,"Chip not detected\n");
        return -1;
    }
    return soft_clk_flag;
}

int main(int argc, char *argv[])
{

    int nBytes = 0;
    int address = 0;
    uint16_t data;

    if(argc<2)
    {
        usage(argv[0]);
        return -1;
    }

    handle = usbOpenDevice(0x16c0,"NeuTroN",0x7755,"93CXXProg");

    if(handle == NULL)
    {
        printf("\nError! Programmer Not found.\n");
        return 0;
    }

    //this checking here detects whether the chip requires soft_clk or not
    //and also determines if any chip exists or not
    if(check_type() == -1)
    {
        usb_close(handle);
        return 0;
    }

    if(stricmp(argv[1],"read")==0)
    {
        if(argc==3 && stricmp(argv[2],"all")==0)
        {
            uint8_t k;
            for(k=0;k<64;k++)
            {
                nBytes = read_word(&data,k);

                //blink led while transmission
                led(k%2);

                if(nBytes<0)
                    fprintf(stderr,"Error!\n");
                else
                    printf("%04x\t",data);
            }
        }

        else
        {
            address = atoi(argv[2]);

            nBytes = read_word(&data,address);

            if(nBytes < 0)
            {
                fprintf(stderr,"Error!");
                usb_close(handle);
                return -1;
            }
            printf("\n%04x is data\n",data);

        }

        //turn off led
        led(USB_LED_OFF);

    }

    else if (argc == 4 && stricmp(argv[1],"write")==0)
    {
        uint16_t address = atoi(argv[2]);
        uint16_t word = atoi(argv[3]);

        nBytes = write_word(word,address);

        if(nBytes<0)
            fprintf(stderr,"Error!\n");
        else
            fprintf(stdout,"\nData written successfully.\n");
    }

    else if(argc > 2 && stricmp(argv[1],"read2f")==0)
    {
        char c;
        printf("\nCaution! if file \"%s\" exists its contents will be overwritten.\
               \nTo Continue press 'y' any other key to abort: ",argv[2]);


            c = getchar();
            c = tolower(c); //convert if upper case to lower

            if(c!='y')
                    return 0;

            FILE *fp = fopen(argv[2],"wb");

            if(!fp)
            {
                fprintf(stderr,"\nCannot create file. Check filepath and name properly.\n");
                return -1;
            }

        uint16_t fdata;
        uint8_t pointer=0;

        for(pointer=0;pointer<64;pointer++)
        {

            nBytes = read_word(&fdata,pointer);

            //blink led while transmission
            led(pointer%2);

            fwrite(&fdata,2,1,fp);

           //printf("%04x",fdata);

            if(nBytes<0)
            {
                fprintf(stderr,"Error! Try Again\n");
                fclose(fp);
                return -1;
            }

        }

        fclose(fp);
        fprintf(stdout,"\nData written to file \"%s\" successfully\n",argv[2]);

        //turn off led
        led(USB_LED_OFF);

    }

    else if(argc == 3 && stricmp(argv[1],"writef")==0)
    {
        FILE *fp = fopen(argv[2],"rb");

        if(!fp)
        {
            fprintf(stderr,"\nCannot open file. Check filepath and name properly.\n");
            return -1;
        }

        uint16_t fdata,pointer=0;
        printf("\nWriting:");//console progress indication

        for(pointer=0;pointer<64;pointer++)
        {
            fread(&fdata,2,1,fp);

            nBytes = write_word(fdata,pointer);

            printf("#");//console progress indication

            //blink led while transmission
            led(pointer%2);

            if(nBytes<0)
            {
                fprintf(stderr,"Error! Try Again\n");
                fclose(fp);
                return -1;
            }
        }

        printf("\nWriting Complete Now Lets Verify Data.");
        printf("\n\nVerifying:");

        fseek(fp,0,SEEK_SET);

        uint8_t count =0;
        for(pointer=0;pointer<64;pointer++)
        {
            printf("#");
            fread(&data,2,1,fp);
            read_word(&fdata,pointer);
            if(data != fdata)
                count++;
            led(pointer%2);
        }

        if(count>0)
            printf("\n%d Error's occurred during verifying.\n",count);

        else
        {
            printf("\nVerifying Complete.");
            printf("\nData in file \"%s\" written Successfully.\n",argv[2]);
        }
        //turn off led
        led(USB_LED_OFF);
    }

    else if(stricmp(argv[1],"on")==0)
    {
            led(USB_LED_ON);
    }
    else if(stricmp(argv[1],"off")==0)
    {
            led(USB_LED_OFF);
    }


    else if(stricmp(argv[1],"list")==0)
    {
        list_all_usb_devs();
    }

    else usage(argv[0]);

    usb_close(handle);

    return 0;
}



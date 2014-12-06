#include<stdio.h>
#include"libusb\include\lusb0_usb.h"

/* This function will be used to get usbDescriptor strings of individual devices
for indentifying which is the device that we are looking for */
static int usbGetDescriptorString(usb_dev_handle * dev, int index, int langid,
                                  char *buf, int buflen)
{
    char buffer[256];
    int rval,i;
 //making a standard request "GET_DESCRIPTOR", which is of type string and given index value
//eg dev->iProduct

    rval = usb_control_msg(dev, USB_TYPE_STANDARD | USB_RECIP_DEVICE | USB_ENDPOINT_IN,
                           USB_REQ_GET_DESCRIPTOR, (USB_DT_STRING << 8) + index,
                           langid, buffer, sizeof(buffer), 1000 );
    //now rval contains the number of bytes read from the device, because that is
    //what usb_control_msg() returns and buffer contains the msg returned by the device

    if(rval<0)
    return rval;//error i.e. error has occured if less than zero is returned

    if((unsigned char)buffer[0] < rval)//rval contains number of bytes read
    rval = (unsigned char)buffer[0];//but buffer[0] contains actual number of bytes the device sent
    //this means string is shorter than bytes read.

    if(buffer[1] != USB_DT_STRING) // second byte is data type
    return 0; //invalid data has been received or returned

    //the data that is received is actually in UTF-16LE format so actual number
    //of characters in the string is half of rval and also index 0 is excluded
    rval /= 2;
    /* now running an algorithm which converts data present in buffer
    to ISO Latin1 format and other characters which donot fall in latin1
    character set are discarded; however please note that i don't understand
    the working of this algorithm completely:only partly*/
    for(i=1;i<rval && i< buflen ; i++)
    {
          if(buffer[2 * i + 1] == 0)
            buf[i-1] = buffer[2 * i];
        else
            buf[i-1] = '?'; /* outside of ISO Latin1 range */
    }
    buf[i-1] = 0;

    return i-1;
}

//this function opens devices one by one and queries if the device descriptor
//strings matches that required by us and returns us the dev handle of
//the device whose descriptor string matches to those given in argument

//cannot use static in here as described in codeandlife.com because i have placed this
//function in a separate file from the main() and accessing it from main
//that is why i declared this in the main as "extern"
usb_dev_handle * usbOpenDevice(int vendor, char *vendorName, int product, char *productName)
{
    struct usb_bus *bus; //define a pointer to a libusb structure which will
    //contain information of all the usb busses(hubs) on the host
    struct usb_device *dev;//define a pointer to a libusb structure which will
    //contain information of all the usb devices attached to the host via any hubs
    char devVendor[256], devProduct[256];

    usb_dev_handle * handle = NULL; //this handle will later on contain the handle to the device
    //we are looking for

    usb_init(); //initialise the libusb functions

    usb_find_busses(); //list all the buses on the system
    usb_find_devices();//list all devices on the system

    for(bus = usb_get_busses(); bus; bus = bus->next)
    {
        for(dev = bus->devices; dev; dev= dev->next)
        {
            if(dev->descriptor.idVendor != vendor ||
               dev->descriptor.idProduct != product)
               continue;
               //there is match in vendor and product ids
               //we need to open the device to query the strings directly
               //from the device to conform.
               if(!(handle =usb_open(dev)))
               {

                   fprintf(stderr,"warning! cannot open the usb_device %sn", usb_strerror());
                   continue;

               }

               //get vendor name, from here you can better understand the usage of usbGetDescriptorSting()
               if(usbGetDescriptorString(handle,dev->descriptor.iManufacturer, 0x0409,
                                         devVendor,sizeof(devVendor)) < 0)//0x0409 is the unicode langid for English(us)
                                         //you might have seen it as 1033 however 0x0409 is 1033 in base10 format
                {
                        fprintf(stderr,"warning! cannot query Manufacturer for device : %sn",usb_strerror());
                        usb_close(handle);
                        continue;
                }

                if(usbGetDescriptorString(handle,dev->descriptor.iProduct,0x0409,
                                          devProduct,sizeof(devProduct)) < 0)
                {
                    fprintf(stderr,"Warning! cannot query Product name for device: %sn",usb_strerror());
                    usb_close(handle);
                    continue;
                }
                //if no errors occurred above in trying to read manufacturer and product strings
                //from the device no comes the real job lets compare them with those given
                //in this function's argument
                if(strcmp(devVendor,vendorName)==0 && strcmp(devProduct,productName)==0)
                {
                    return handle;
                }

                else usb_close(handle);
        }

    }

    return NULL; //i.e. device not found
}

void list_all_usb_devs()
{
     struct usb_bus *bus; //define a pointer to a libusb structure which will
    //contain information of all the usb busses(hubs) on the host
    struct usb_device *dev;//define a pointer to a libusb structure which will
    //contain information of all the usb devices attached to the host via any hubs
    char devVendor[256], devProduct[256];

    usb_dev_handle * handle = NULL; //this handle will later on contain the handle to the device
    //we are looking for

    usb_init(); //initialise the libusb functions

    usb_find_busses(); //list all the buses on the system
    usb_find_devices();//list all devices on the system

    for(bus = usb_get_busses(); bus; bus = bus->next)
    {
        for(dev = bus->devices; dev; dev= dev->next)
        {
           fprintf(stdout,"VID_%x&PID_%x\n",dev->descriptor.idVendor,dev->descriptor.idProduct);
               if(!(handle =usb_open(dev)))
               {

                   fprintf(stderr,"warning! cannot open the usb_device %sn", usb_strerror());
                   continue;

               }

               //get vendor name, from here you can better understand the usage of usbGetDescriptorSting()
               if(usbGetDescriptorString(handle,dev->descriptor.iManufacturer, 0x0409,
                                         devVendor,sizeof(devVendor)) < 0)//0x0409 is the unicode langid for English(us)
                                         //you might have seen it as 1033 however 0x0409 is 1033 in base10 format
                {
                        fprintf(stderr,"warning! cannot query Manufacturer for device : %sn",usb_strerror());
                        usb_close(handle);
                        continue;
                }

                if(usbGetDescriptorString(handle,dev->descriptor.iProduct,0x0409,
                                          devProduct,sizeof(devProduct)) < 0)
                {
                    fprintf(stderr,"Warning! cannot query Product name for device: %sn",usb_strerror());
                    usb_close(handle);
                    continue;
                }
                //if no errors occurred above in trying to read manufacturer and product strings
                //from the device no comes the real job lets compare them with those given
                //in this function's argument
               fprintf(stdout,"Manufacturer Name = %s and Product Name = %s\n",devVendor,devProduct);
            usb_close(handle);

        }

    }
//by building this function i found that libusb cannot talk with all types of USB
//devices directly. To talk with the devices using the functions like usb_control_msg()
//the devices should have a libusb driver installed only these functions will work on the
//device. Well thats what i know as far as now.

    return;
}

#include"libusb.h"
#include<iostream>
#include<cstring>
#include<array>
using namespace std;
const uint16_t vid = 0x403;
const uint16_t pid = 0x6001;

class usb_comm
{
public:
    //打开usb通讯端口
    int open()
    {
        return 0;
    } 
    //从设备读数据
    int read()
    {
        return 0;
    }
    //向设备写数据
    int write()
    {
        return 0;
    }
    //关闭usb通讯端口
    int close()
    {
        return 0;
    }
private:
    //libusbAPI 只初始化一次
    class usb_comm_helper
    {
    public:
        usb_comm_helper()
        {
            libusb_init(nullptr);
        }
        ~usb_comm_helper()
        {
            libusb_exit(nullptr);
        }
    };
private:
    libusb_context *_context = nullptr;
    libusb_device_handle *handle = nullptr;
    uint16_t _vid = 0x403 ;
    uint16_t _pid =  0x6001;
    uint8_t _endpoint_write = 0;
    uint8_t _endpoint_read = 0;
//    static usb_comm_helper s_usb_comm_helper;
};

int main()
{
    cout<<"是否支持热插拔: "<<libusb_has_capability(LIBUSB_CAP_HAS_HOTPLUG)<<endl;
    cout<<"是否支持HID设备："<<libusb_has_capability(LIBUSB_CAP_HAS_HID_ACCESS)<<endl;
    cout<<"是否支持释放默认驱动: "<<libusb_has_capability(LIBUSB_CAP_SUPPORTS_DETACH_KERNEL_DRIVER)<<endl;
    cout<<"API是否有效："<<libusb_has_capability(LIBUSB_CAP_HAS_CAPABILITY)<<endl;
//    libusb_hotplug_register_callback()   USB设备热插拔
    libusb_context *context = nullptr;
    int ret = libusb_init(&context);
    if(0 != ret)
    {
        cout<<"libusb_init() failed!"<<endl;
        return 0;       
    }
    cout<<"打印设备描述符"<<endl;
    libusb_device **dev_list = nullptr;  
    libusb_device *dev = nullptr;
    libusb_get_device_list(context,&dev_list);

    int i = 0;
    libusb_device_handle *handle = nullptr;
    while(((dev = dev_list[i++]))!=nullptr)
    {
        libusb_device_descriptor dev_descriptor;
        libusb_get_device_descriptor(dev,&dev_descriptor);
        cout<<"VID = 0x"<<hex<<dev_descriptor.idVendor<<endl;
        cout<<"PID = 0x"<<hex<<dev_descriptor.idProduct<<endl;
        cout<<"==================================================="<<endl;
        if(vid == dev_descriptor.idVendor && pid == dev_descriptor.idProduct)
        {
            int status = libusb_open(dev,&handle);
            if(0 != status)
            {
                cout<<"打开设备失败! :"<<status<<endl;
                continue;
            }
            cout<<"打开设备成功!"<<endl;
            unsigned char serialNumber[50]={'\0'};
            libusb_get_string_descriptor_ascii(handle,dev_descriptor.iSerialNumber,serialNumber,sizeof(serialNumber)/sizeof(serialNumber[0]));
            unsigned char product[50]={'\0'};
            cout<<"serialNumber = "<<serialNumber<<endl;
            libusb_get_string_descriptor_ascii(handle,dev_descriptor.idProduct,product,sizeof(product)/sizeof(product[0]));
            cout<<"product = "<<product<<endl;
            unsigned char vendor[50]={'\0'};
            libusb_get_string_descriptor_ascii(handle,dev_descriptor.idVendor,vendor,sizeof(vendor)/sizeof(vendor[0]));
            cout<<"vendor = "<<vendor<<endl;
            unsigned char manufacturer[50]={'\0'};
            libusb_get_string_descriptor_ascii(handle,dev_descriptor.iManufacturer,manufacturer,sizeof(manufacturer)/sizeof(manufacturer[0]));
            cout<<"manufacturer = "<<manufacturer<<endl;
            cout<<"the number of configurations = "<<(int)dev_descriptor.bNumConfigurations<<endl;
            break;
        }
    }
    if(nullptr == handle) 
    {
        cout<<"open usb failed!"<<endl;
        return 0;
    }
    cout<<"open usb success"<<endl;
    //获取当前成功打开的设备的配置描述符
    libusb_config_descriptor *config_descriptor = nullptr;
    const libusb_endpoint_descriptor *endpoint_descriptor = nullptr;
    uint8_t endpoint_write = 0,endpoint_read = 0;
    libusb_get_config_descriptor(dev,0,&config_descriptor);
    //获取当前配置描述符下的接口数
    uint8_t interfaceNumber = config_descriptor->bNumInterfaces;
    cout<<"当前配置描述下接口数："<<(int)interfaceNumber<<endl;
    for(uint8_t i = 0;i<interfaceNumber;i++)
    {   
        cout<<"当前端点的备用："<< config_descriptor->interface[i].num_altsetting<<endl;
        for(int j = 0 ;j<config_descriptor->interface[i].num_altsetting;j++)
        {                                               //接口        //接口描述符
            uint8_t interfaceClass = config_descriptor->interface[i].altsetting[j].bInterfaceClass;
            cout<<"当前端点的类别："<<(int)interfaceClass<<endl;
            if(0xFF != interfaceClass && 0x0A != interfaceClass) continue;
            cout<<"当前接口的端点数："<<(int)config_descriptor->interface[i].altsetting[j].bNumEndpoints<<endl;
            for(uint8_t k = 0;k<config_descriptor->interface[i].altsetting[j].bNumEndpoints;k++)
            {
                //获取端点描述符
                endpoint_descriptor = &config_descriptor->interface[i].altsetting[j].endpoint[k];
                if(endpoint_descriptor->bmAttributes & LIBUSB_TRANSFER_TYPE_MASK & LIBUSB_TRANSFER_TYPE_BULK)
                {
                    //读端口
                    if(endpoint_descriptor->bEndpointAddress & LIBUSB_ENDPOINT_IN)
                    {
                        endpoint_read = endpoint_descriptor->bEndpointAddress;   
                        if(0 != endpoint_read)
                        {
                            libusb_clear_halt(handle,endpoint_read);//清除暂停标志
                            /*
                            libusb_bulk_transfer()返回 LIBUSB_ERROR_PIPE 时调用libusb_clear_halt（）API
                            */
                        }   
                    }
                    else
                    {
                        endpoint_write = endpoint_descriptor->bEndpointAddress;
                        if(endpoint_write != 0)
                        {
                            libusb_clear_halt(handle,endpoint_write);
                        }
                    }
                }  
            }
        }
    }
    //释放设备描述符
    libusb_free_config_descriptor(config_descriptor);
    /*
        libusb_set_auto_detach_kernel_driver（）:设置自动卸载内核驱动，但没有执行具体行动，待
        调用libusb_claim_interface()api时才会卸载内核驱动。
        libusb_release_interface（）自动加载内核驱动  
        libusb_attach_kernel_driver()自动加载接口的内核驱动
        libusb_kernel_driver_active()判定接口的内核驱动是否激活 若激活则libusb_claim_interface()申请接口会失败
        libusb_detach_kernel_driver()卸载接口的内核驱动
    */
    libusb_set_auto_detach_kernel_driver(handle,1);
    for(uint8_t i = 0;i<interfaceNumber;i++)
    {
        int status = libusb_claim_interface(handle,i);
        if(LIBUSB_SUCCESS != status)
        {
            cout<<"声明接口失败"<<endl;
        }
        else
        {
            cout<<"声明接口成功"<<endl;
        }
    }
    libusb_free_device_list(dev_list,1);
    if(endpoint_read == 0 || endpoint_write == 0)
    {
        cout<<"未找到读写端点"<<endl;
        return 0;
    }
    libusb_ref_device(dev);//重新初始化
    cout<<"endpoint_write = "<< (int)endpoint_write <<" endpoint_read = "<< (int)endpoint_read <<endl;

    //写数据
    unsigned char sendData[4]={0xD4,0x89,0x0d,0xf7};
    int sendLength = 0;
    int write_status = libusb_bulk_transfer(handle,endpoint_write,sendData,sizeof(sendData)/sizeof(sendData[0]),&sendLength,0);
    cout<<"send_bytes :"<<sendLength<<endl;
    if(write_status<0)
    {
        cout<<"write data error :"<<libusb_strerror(write_status);
        return 0;
    }

    //读数据
    unsigned char recv_data[100]={'\0'};
    int recv_length = 0;
    int read_status = libusb_bulk_transfer(handle,endpoint_read,recv_data,sizeof(recv_data)/sizeof(recv_data[0]),&recv_length,0);
    //为什么接受的数据开头都是0x31 0x60,后面的数据是正常的数据
    if(read_status<0)
    {
        cout<<"read data error :"<<libusb_strerror(read_status);
        return 0;
    }
    cout<<"recv_bytes: "<<recv_length<<endl;
    cout<<"recv_data : ";
    for(int i = 0;i<recv_length;i++)
    {
        cout<<(int)recv_data[i]<<" ";
    }
    cout<<endl;

    //关闭设备
    for(uint8_t i = 0;i<interfaceNumber;i++)
    {
        libusb_release_interface(handle,i);
    }
    libusb_close(nullptr);
    //
    libusb_exit(context);   
    cout<<"mission success!"<<endl;
    return 0;
}
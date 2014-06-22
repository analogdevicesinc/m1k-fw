#ifndef _MAIN_H_
#define _MAIN_H_

bool main_vendor_enable(void);

void main_vendor_disable(void);

void main_sof_action(void);

void main_suspend_action(void);

void main_resume_action(void);

bool main_setup_handle(void);

void init_build_usb_serial_number(void);

void hardware_init(void);

#endif // _MAIN_H_

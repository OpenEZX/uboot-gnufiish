
#include <common.h>
#include <usbdcore.h>
#include <s3c2410.h>

int udc_usb_maxcurrent = 0;

void udc_ctrl(enum usbd_event event, int param)
{
	S3C24X0_GPIO * const gpio = S3C24X0_GetBase_GPIO();

	switch (event) {
	case UDC_CTRL_PULLUP_ENABLE:
		if (param)
			gpio->GPADAT |= (1 << 21);	/* GPA21 */
		else
			gpio->GPADAT &= ~(1 <<21);
		break;
	case UDC_CTRL_500mA_ENABLE:
		/* FIXME */
		if (param)
			udc_usb_maxcurrent = 500;
		else
			udc_usb_maxcurrent = 0;
		break;
	default:
		break;
	}
}

/*******************************************************************************
*************************keySTREAM Trusted Agent ("KTA")************************

* (c) 2023-2024 Nagravision Sàrl

* Subject to your compliance with these terms, you may use the Nagravision Sàrl
* Software and any derivatives exclusively with Nagravision's products. It is your
* responsibility to comply with third party license terms applicable to your
* use of third party software (including open source software) that may accompany
* Nagravision Software.

* Redistribution of this Nagravision Software in source or binary form is allowed
* and must include the above terms of use and the following disclaimer with the
* distribution and accompanying materials.

* THIS SOFTWARE IS SUPPLIED BY NAGRAVISION "AS IS". NO WARRANTIES, WHETHER EXPRESS,
* IMPLIED OR STATUTORY, APPLY TO THIS SOFTWARE, INCLUDING ANY IMPLIED WARRANTIES OF
* NON-INFRINGEMENT, MERCHANTABILITY, AND FITNESS FOR A PARTICULAR PURPOSE. IN NO
* EVENT WILL NAGRAVISION BE LIABLE FOR ANY INDIRECT, SPECIAL, PUNITIVE, INCIDENTAL
* OR CONSEQUENTIAL LOSS, DAMAGE, COST OR EXPENSE OF ANY KIND WHATSOEVER RELATED TO
* THE SOFTWARE, HOWEVER CAUSED, EVEN IF NAGRAVISION HAS BEEN ADVISED OF THE
* POSSIBILITY OR THE DAMAGES ARE FORESEEABLE. TO THE FULLEST EXTENT ALLOWED BY LAW,
* NAGRAVISION 'S TOTAL LIABILITY ON ALL CLAIMS IN ANY WAY RELATED TO THIS
* SOFTWARE WILL NOT EXCEED THE AMOUNT OF FEES, IF ANY, THAT YOU HAVE PAID DIRECTLY
* TO NAGRAVISION FOR THIS SOFTWARE.
********************************************************************************/
/** \brief Communication stack interface utility.
 *
 *  \author Kudelski IoT
 *
 *  \date 2023/06/12
 *
 *  \file comm_interface_util.c
 ******************************************************************************/
/**
 * @brief Communication stack interface utility.
 */

#include "comm_interface_util.h"

/* -------------------------------------------------------------------------- */
/* IMPORTS                                                                    */
/* -------------------------------------------------------------------------- */

/* -------------------------------------------------------------------------- */
/* LOCAL CONSTANTS, TYPES, ENUM                                               */
/* -------------------------------------------------------------------------- */

/* Maximum number of dots. */
#define C_COMM_INTERFACE_COAP_IS_IPV4_ADDRESS (3u)

/* -------------------------------------------------------------------------- */
/* LOCAL VARIABLES                                                            */
/* -------------------------------------------------------------------------- */

/* -------------------------------------------------------------------------- */
/* LOCAL FUNCTIONS - PROTOTYPE                                                */
/* -------------------------------------------------------------------------- */

/* -------------------------------------------------------------------------- */
/* PUBLIC VARIABLES                                                           */
/* -------------------------------------------------------------------------- */

/* -------------------------------------------------------------------------- */
/* PUBLIC FUNCTIONS - IMPLEMENTATION                                          */
/* -------------------------------------------------------------------------- */

/**
 * @brief  implement commUtilConvertSocketIp
 *
 */
TBoolean commUtilConvertSocketIp
(
  const uint8_t*  xpAddress,
  TKSocketIp*     xpSocketIp
)
{
  size_t value = 0;
  uint32_t address = 0;
  uint8_t numDots = 0;
  size_t index = 0;
  TBoolean isPort = E_FALSE;
  TBoolean isEnd = E_FALSE;
  TBoolean isValid = E_FALSE;

  if (NULL != xpAddress)
  {
    isValid = E_TRUE;

    for (index = 0; isValid && !isEnd; index++)
    {
      switch (xpAddress[index])
      {
        case '\0':
          isEnd = E_TRUE;

          if (isPort == E_TRUE)
          {
            if ((value > 0u) && (value <= 0xFFFFu))
            {
              /* format is a.b.c.d:port */
              xpSocketIp->address.v4.port = (uint16_t)value;
            }
            else
            {
              isValid = E_FALSE;
            }
          }
          else
          {
            if (C_COMM_INTERFACE_COAP_IS_IPV4_ADDRESS == numDots)
            {
              /* format is a.b.c.d */
              xpSocketIp->address.v4.address = (address << 8u) + (uint32_t)value;
            }
            else
            {
              isValid = E_FALSE;
            }
          }

          break;

        case ':':
          isPort = E_TRUE;

          switch (numDots)
          {
            case 0: /* format is ":port" */
              break;

            case 3: /* format is "a.b.c.d:" */
              xpSocketIp->address.v4.address = (address << 8u) + (uint32_t)value;
              value = 0;
              break;

            default:
              isValid = E_FALSE;
              break;
          }

          break;

        case '.':
          if ((isPort) || (numDots > C_COMM_INTERFACE_COAP_IS_IPV4_ADDRESS))
          {
            isValid = E_FALSE;
          }
          else
          {
            numDots++;
            address = (address << 8u) + (uint32_t)value;
            value = 0;
          }

          break;

        default:
          if ((xpAddress[index] >= (uint8_t)'0') && (xpAddress[index] <= (uint8_t)'9'))
          {
            value = (10U * value) + (size_t)(xpAddress[index] - (uint8_t)'0');
          }
          else
          {
            isValid = E_FALSE;
          }

          break;
      }
    }
  }

  if (isValid == E_TRUE)
  {
    xpSocketIp->protocol = E_K_IP_PROTOCOL_V4;
  }

  return isValid;
}

/* -------------------------------------------------------------------------- */
/* LOCAL FUNCTIONS - IMPLEMENTATION                                           */
/* -------------------------------------------------------------------------- */

/* -------------------------------------------------------------------------- */
/* END OF FILE                                                                */
/* -------------------------------------------------------------------------- */

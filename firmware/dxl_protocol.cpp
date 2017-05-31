#include <wirish/wirish.h>
#include "dxl_protocol.h"

void dxl_process(
        volatile struct dxl_device *device,
        volatile struct dxl_packet *packet,
        bool (*dxl_check_id)(volatile struct dxl_device *self, ui8 id),
        void (*dxl_write_data)(volatile struct dxl_device *self, ui8 id, ui8 addr, ui8 *values, ui8 length),
        void (*dxl_read_data)(volatile struct dxl_device *self, ui8 id, ui8 addr, ui8 *values, ui8 length, ui8 *error)
        )
{
    if (dxl_check_id(device, packet->id) || packet->id == DXL_BROADCAST) {
        device->packet.id = packet->id;

        switch (packet->instruction) {
            case DXL_PING: 
                // Answers the ping
                if (packet->id != DXL_BROADCAST) {
                    device->packet.error = DXL_NO_ERROR;
                    device->packet.parameter_nb = 0;
                    device->packet.process = true;
                }
                break;

            case DXL_WRITE_DATA:
                // Write data
                dxl_write_data(device,
                        packet->id,
                        packet->parameters[0],
                        (ui8 *)&packet->parameters[1],
                        packet->parameter_nb-1);
                break;

            case DXL_SYNC_WRITE: {
                                     ui8 addr = packet->parameters[0];
                                     int length = packet->parameters[1] + 1;
                                     int K = (packet->parameter_nb-2) / length;
                                     int i;

                                     for (i=0; i<K; i++) {
                                         if (dxl_check_id(device, packet->parameters[2+i*length])) {
                                             dxl_write_data(device, packet->parameters[2+i*length],
                                                     addr,
                                                     (ui8 *)&packet->parameters[2+i*length+1],
                                                     (ui8)(length-1));
                                         }
                                     }
                                 }
                                 break;

            case DXL_READ_DATA:
                                 // Read some data
                                 if (packet->id != DXL_BROADCAST) {
                                     ui8 addr = packet->parameters[0];
                                     unsigned int length = packet->parameters[1];

                                     if (length < sizeof(packet->parameters)) {
                                         device->packet.process = true;
                                         dxl_read_data(device, packet->id, addr, (ui8 *)device->packet.parameters, length, (ui8 *)&device->packet.error);

                                         device->packet.parameter_nb = length;
                                     }
                                 }
        }
    }
}

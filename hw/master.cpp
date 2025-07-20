#include "systemc.h"
#include "tlm.h"
#include "tlm_utils/simple_initiator_socket.h"

SC_MODULE(Master)
{
    tlm_utils::simple_initiator_socket<Master, 32> initiator_socket;

    SC_CTOR(Master) : initiator_socket("initiator_socket") {}
};

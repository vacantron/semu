#include "systemc.h"
#include "tlm.h"
#include "tlm_utils/simple_target_socket.h"

#define DRAM_SIZE (0x200)

SC_MODULE(Slave)
{
    tlm_utils::simple_target_socket<Slave, 32> target_socket;

    uint32_t mem[DRAM_SIZE];

    SC_CTOR(Slave) : target_socket("target_socket")
    {
        target_socket.register_b_transport(this, &Slave::b_transport);

        for (int i = 0; i < DRAM_SIZE; ++i) {
            mem[i] = 0;
        }
    }

    void b_transport(tlm::tlm_generic_payload & gp, sc_time & delay)
    {
        tlm::tlm_command cmd = gp.get_command();
        sc_dt::uint64 addr = gp.get_address();
        uint32_t *data_ptr = reinterpret_cast<uint32_t *>(gp.get_data_ptr());

        delay += sc_time(10, SC_NS);

        switch (cmd) {
        case tlm::TLM_READ_COMMAND:
            data_ptr[0] = mem[addr & (DRAM_SIZE - 1u)];
            printf("SystemC: Reading from %#x(data: %#x)\n",
                   (uint32_t) addr & (DRAM_SIZE - 1), data_ptr[0]);
            break;
        case tlm::TLM_WRITE_COMMAND:
            mem[addr & (DRAM_SIZE - 1u)] = data_ptr[0];
            printf("SystemC: Writing to %#x(data: %#x)\n",
                   (uint32_t) addr & (DRAM_SIZE - 1), data_ptr[0]);
            break;
        default:
            gp.set_response_status(tlm::TLM_COMMAND_ERROR_RESPONSE);
            return;
        }

        gp.set_response_status(tlm::TLM_OK_RESPONSE);
    }
};
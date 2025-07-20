#include "systemc"

#include "master.cpp"
#include "slave.cpp"

Master *master = nullptr;
Slave *slave = nullptr;

extern "C" {

void systemc_dram_init()
{
    master = new Master("master");
    slave = new Slave("slave");

    master->initiator_socket.bind(slave->target_socket);
    sc_core::sc_start(sc_core::SC_ZERO_TIME);
}

uint32_t systemc_dram_read(uint32_t addr)
{
    uint32_t data[1];
    sc_time delay;
    tlm::tlm_generic_payload gp;

    gp.set_command(tlm::TLM_READ_COMMAND);
    gp.set_address(addr);
    gp.set_data_ptr(reinterpret_cast<unsigned char *>(data));

    master->initiator_socket->b_transport(gp, delay);

    return data[0];
}

void systemc_dram_write(uint32_t addr, uint32_t val)
{
    uint32_t data[1];
    sc_time delay;
    tlm::tlm_generic_payload gp;

    data[0] = val;

    gp.set_command(tlm::TLM_WRITE_COMMAND);
    gp.set_address(addr);
    gp.set_data_ptr(reinterpret_cast<unsigned char *>(data));

    master->initiator_socket->b_transport(gp, delay);
}
}

int sc_main(int argc, char *argv[])
{
    return 0;
}

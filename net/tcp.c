#include "tcp.h"

void tcp_header_size_check(void)
{
	switch(0) {case 0:case sizeof(tcp_header_t) == 20:;}
}

uint16_t tcp_hdr_len(tcp_header_t* tcph)
{
	return (tcph->dataofs & 0xf0) >> 2;
}

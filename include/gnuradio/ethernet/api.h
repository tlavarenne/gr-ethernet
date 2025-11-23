#ifndef INCLUDED_ETHERNET_API_H
#define INCLUDED_ETHERNET_API_H

#include <gnuradio/attributes.h>

#ifdef gnuradio_ethernet_EXPORTS
#define ETHERNET_API __GR_ATTR_EXPORT
#else
#define ETHERNET_API __GR_ATTR_IMPORT
#endif

#endif

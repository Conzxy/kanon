#ifndef KANON_INET_ADDR_H
#define KANON_INET_ADDR_H

#include <vector>
#include <arpa/inet.h>
#include <netinet/in.h>
#include <stdint.h>

#include "kanon/util/macro.h"
#include "kanon/string/string_view.h"

/*
 * Generic socket address(old but also useful)
 * struct sockaddr {
 *    sa_family_t sa_family;
 *    char        sa_data[14];
 * }
 *
 * IPv4 socket address structure
 * struct sockaddr_in {
 *     sa_family_t     sin_family;
 *     in_port_t       sin_port;
 *     struct in_addr  sin_addr;
 * };
 *
 * struct in_addr {
 *     in_addr_t s_addr;
 * };
 *
 * IPv6 socket address structure
 * struct sockaddr_in6 {
 *     sa_family_t     sin6_family;
 *     in_port_t       sin6_port;
 *     uint32_t        sin6_flowinfo;
 *     struct in6_addr sin6_addr;
 *     uint32_t        sin6_scoped_id;
 * };
 *
 * struct in6_addr {
 *     unsigned char s6_addr[16];
 * };
 *
 * obviously, the offset of family and port field is same,
 * so in union, we use these field which no need to distinguish.
 *
 * struct addrinfo {
 *   int                ai_flags;     // Hints argument flags
 *   int                ai_family;    // specified ip family(1st arg)
 *   int                ai_socktype;  // specified socket type(2nd arg)
 *   int                ai_protocol;  // specified protocol(3rd arg)
 *   socklen_t          ai_addrlen;   // address length
 *   struct  sockaddr*  ai_addr;      // point to the sockaddr structure
 *   char*              ai_canonname; // official name
 *   struct addrinfo*   ai_next;      // point to next item in linked list
 * };
 */

struct addrinfo;

namespace kanon {

/**
 * \addtogroup net
 * \brief Network module, including event loop in reactor mode and tcp server/client, etc
 */
//!@{

/**
 * \brief Represent a internet address(Ipv4 or Ipv6)
 * 
 * In fact, this is wrapper of sockaddr_in or sockaddr_in6
 */
class InetAddr {
public:
  using Port = uint16_t;

  /**
   * \brief Construct address from the hostname and service(port)
   * \param hostname
   *   This is either a hostname(domain name) or an address string
   *   (dotted-decimal for Ipv4 or hex string for Ipv6)
   * \param service
   *   This is either a service name(e.g. http) or a decimal port
   *   number(16bit)
   * \note
   *   I recommend to use InetAddress(address)
   */
  InetAddr(StringArg hostname, StringArg service);

  /**
   * \brief Construct address from the port
   * \param port
   *   A 16-bit decimal number, which is used for binding socket
   * \param loopback
   * 
   * \param v6
   *   As a Ipv6 address if ths machine support
   * \warning
   *   This must be used for server
   */ 
  explicit InetAddr(Port port=0, bool loopback=false, bool v6=false);

  /**
   * \brief Construct address from ip address and port number
   * \param ip
   *   A string representation of Ipv4 or Ipv6 address
   * \param port
   *   A 16-bit decimal port number
   * \note
   *   If want to use hostname and service, don't use this
   */
  InetAddr(StringView ip, Port port);
  
  /**
   * \brief Construct address from address string
   * \param address 
   *   Peer address including ip address and port number
   *     - Ipv4 IP address:port
   *     - [Ipv6 IP address]:port   
   *     - hostname:port
   */
  explicit InetAddr(StringView address);

  /**
   * \brief Compatible with C struct sockaddr_in
   * \note
   *   Support implicit coversion
   */
  InetAddr(struct sockaddr_in addr)
    : addr_{ addr }
  { }

  /**
   * \brief Compatible with C struct sockaddr_in6
   * \note
   *   Support implicit coversion
   */
  InetAddr(struct sockaddr_in6 addr6)
    : addr6_{ addr6 }
  { }

  //! \name Conversion
  //!@{
  /**
   * \brief Convert to string that represent ip and port
   * \return
   *   A string represent ip address and port number.
   *   The form is [ip address;port number].
   *   The reason for don't use ":" to split two parts
   *   is ipv6 contains ":".
   */
  std::string ToIpPort() const;

  /**
   * \brief Convert to string that represent ip
   * \return
   *   A string represent ip address
   */
  std::string ToIp() const;  
  
  /**
   * \brief Convert to Ipv4 address(sockaddr_in)
   * \note
   * If this address is not a Ipv4 address in fact,
   * just abort
   */
  struct sockaddr_in const* ToIpv4() const noexcept
  { 
    KANON_ASSERT(IsIpv4(), "The InetAddr doesn't represent an ipv4 address");
    return &addr_; 
  }
  
  /**
   * \brief Convert to Ipv6 address(sockaddr_in6)
   * \note
   * If this address is not a Ipv6 address in fact,
   * just abort.
   */
  struct sockaddr_in6 const* ToIpv6() const noexcept
  { 
    KANON_ASSERT(!IsIpv4(), "The InetAddr doesn't represent an ipv6 address");
    return &addr6_;
  }

  struct sockaddr* ToSockaddr() noexcept
  { return reinterpret_cast<sockaddr*>(&addr_); }

  //!@}

  //! \name Attribute getter  
  //!@{
  
  //! Get the inet family
  sa_family_t GetFamily() const noexcept
  { return addr_.sin_family; }
  
  //! Get the decimal port number 
  Port GetPort() const noexcept;
  
  //! Check whether this is a Ipv4 address 
  bool IsIpv4() const noexcept
  { return addr_.sin_family == AF_INET; }
  //!@}

  //! \name DNS lookup
  //!@{
  /**
   * Resolve hostname
   * \param hostname
   *   This is either a hostname(domain name) or an address string
   *   (dotted-decimal for Ipv4 or hex string for Ipv6)
   * \param service
   *   This is either a service name(e.g. http) or a decimal port
   *   number(16bit)
   * \param is_server
   *   If this is specified as true, return the addresses that are 
   *   suitable for binding, otherwise, return the addresses that 
   *   are suitable for connecting.
   */
  static std::vector<InetAddr> Resolve(StringArg hostname, 
                                       StringArg service, 
                                       bool is_server = false);

  /**
   * This is a HACK method. \n
   * User can fill the @p hint to get specific address list.
   * \param hostname
   *   This is either a hostname(domain name) or an address string
   *   (dotted-decimal for Ipv4 or hex string for Ipv6)
   * \param service
   *   This is either a service name(e.g. http) or a decimal port
   *   number(16bit)
   * \param hint see man getaddrinfo
   */
  static std::vector<InetAddr> Resolve(StringArg hostname, 
                                       StringArg service,
                                       struct addrinfo const* hint);
  //!@}

private:

  /**
   * I don't use sockaddr_storage to implemete it
   * since sockaddr_storage occupy 128 bytes but
   * sockaddr_in6 just 28 bytes and satisfy all
   * requirements.
   */
  union {
    sockaddr_in addr_;   //!< Ipv4 address
    sockaddr_in6 addr6_; //!< Ipv6 address
  };
};

//!@}

} // namespace kanon

#endif // KANON_INET_ADDR_H

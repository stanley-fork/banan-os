#include <kernel/Networking/NetworkManager.h>
#include <kernel/Networking/NetworkSocket.h>

#include <net/if.h>

namespace Kernel
{

	NetworkSocket::NetworkSocket(NetworkLayer& network_layer, const Socket::Info& info)
		: Socket(info)
		, m_network_layer(network_layer)
	{ }

	NetworkSocket::~NetworkSocket()
	{
	}

	bool NetworkSocket::can_interface_send_to(const NetworkInterface& interface, const sockaddr* target, socklen_t target_len) const
	{
		ASSERT(target);
		ASSERT(target_len >= static_cast<socklen_t>(sizeof(sockaddr_in)));
		ASSERT(target->sa_family == AF_INET);

		const auto target_ipv4 = BAN::IPv4Address {
			reinterpret_cast<const sockaddr_in*>(target)->sin_addr.s_addr
		};

		switch (interface.type())
		{
			case NetworkInterface::Type::Ethernet:
				// FIXME: this is not really correct :D
				return target_ipv4.octets[0] != IN_LOOPBACKNET;
			case NetworkInterface::Type::Loopback:
				return target_ipv4.octets[0] == IN_LOOPBACKNET;
		}

		ASSERT_NOT_REACHED();
	}

	BAN::ErrorOr<BAN::RefPtr<NetworkInterface>> NetworkSocket::interface(const sockaddr* target, socklen_t target_len)
	{
		ASSERT(m_network_layer.domain() == NetworkSocket::Domain::INET);
		ASSERT(is_bound());

		if (target != nullptr)
		{
			ASSERT(target_len >= static_cast<socklen_t>(sizeof(sockaddr_in)));
			ASSERT(target->sa_family == AF_INET);
		}

		const auto& all_interfaces = NetworkManager::get().interfaces();

		const auto bound_ipv4 = BAN::IPv4Address {
			reinterpret_cast<const sockaddr_in*>(&m_address)->sin_addr.s_addr
		};

		// find the bound interface
		if (bound_ipv4 != 0)
		{
			for (const auto& interface : all_interfaces)
			{
				const auto netmask = interface->get_netmask();
				if (bound_ipv4.mask(netmask) != interface->get_ipv4_address().mask(netmask))
					continue;
				if (target && !can_interface_send_to(*interface, target, target_len))
					continue;
				return interface;
			}

			return BAN::Error::from_errno(EADDRNOTAVAIL);
		}

		// try to find an interface in the same subnet as target
		if (target != nullptr)
		{
			const auto target_ipv4 = BAN::IPv4Address {
				reinterpret_cast<const sockaddr_in*>(target)->sin_addr.s_addr
			};

			for (const auto& interface : all_interfaces)
			{
				const auto netmask = interface->get_netmask();
				if (target_ipv4.mask(netmask) == interface->get_ipv4_address().mask(netmask))
					return interface;
			}
		}

		// return any interface (prefer non-loopback)
		for (const auto& interface : all_interfaces)
			if (interface->type() != NetworkInterface::Type::Loopback)
				if (!target || can_interface_send_to(*interface, target, target_len))
					return interface;
		for (const auto& interface : all_interfaces)
			if (interface->type() == NetworkInterface::Type::Loopback)
				if (!target || can_interface_send_to(*interface, target, target_len))
					return interface;

		return BAN::Error::from_errno(EHOSTUNREACH);
	}

	void NetworkSocket::bind_address_and_port(const sockaddr* addr, socklen_t addr_len)
	{
		ASSERT(!is_bound());
		ASSERT(addr->sa_family != AF_UNSPEC);
		ASSERT(addr_len <= static_cast<socklen_t>(sizeof(sockaddr_storage)));

		memcpy(&m_address, addr, addr_len);
		m_address_len = addr_len;
	}

	BAN::ErrorOr<long> NetworkSocket::ioctl_impl(int request, void* arg)
	{
		switch (request)
		{
			case SIOCGIFADDR:
			case SIOCSIFADDR:
			case SIOCGIFNETMASK:
			case SIOCSIFNETMASK:
			case SIOCGIFGWADDR:
			case SIOCSIFGWADDR:
			case SIOCGIFHWADDR:
			case SIOCGIFNAME:
				return TRY(interface(nullptr, 0))->ioctl(request, arg);
		}

		return Socket::ioctl_impl(request, arg);
	}

	BAN::ErrorOr<void> NetworkSocket::getsockname_impl(sockaddr* address, socklen_t* address_len)
	{
		TRY(m_network_layer.get_socket_address(this, address, address_len));
		return {};
	}

}

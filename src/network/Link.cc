/*
 *  Multi2Sim
 *  Copyright (C) 2014  Amir Kavyan Ziabari (aziabari@ece.neu.edu)
 *
 *  This program is free software; you can redistribute it and/or modify
 *  it under the terms of the GNU General Public License as published by
 *  the Free Software Foundation; either version 2 of the License, or
 *  (at your option) any later version.
 *
 *  This program is distributed in the hope that it will be useful,
 *  but WITHOUT ANY WARRANTY; without even the implied warranty of
 *  MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
 *  GNU General Public License for more details.
 *
 *  You should have received a copy of the GNU General Public License
 *  along with this program; if not, write to the Free Software
 *  Foundation, Inc., 59 Temple Place, Suite 330, Boston, MA  02111-1307  USA
 */

#include <algorithm>

#include <lib/esim/Event.h>

#include "Node.h"
#include "Link.h"

namespace net
{

Link::Link(Network *network,
		const std::string &name,
		const std::string &descriptive_name,
		Node *src_node,
		Node *dst_node,
		int bandwidth,
		int source_buffer_size,
		int destination_buffer_size,
		int num_virtual_channels) :
						Connection(descriptive_name, network, bandwidth),
						source_node(src_node),
						destination_node(dst_node),
						num_virtual_channels(num_virtual_channels),
						name(name)
{
	for (int i = 0; i < num_virtual_channels ; i++)
	{
		// Create and add source buffer
		Buffer *source_buffer = src_node->addOutputBuffer(
				source_buffer_size, this);
		addSourceBuffer(source_buffer);

		// Create and add destination buffer
		Buffer *destination_buffer = dst_node->addInputBuffer(
				destination_buffer_size, this);
		addDestinationBuffer(destination_buffer);
	}

}


void Link::Dump(std::ostream &os) const
{
	// Dumping user assigned name
	os << misc::fmt("[ Network.%s.Link.%s ]\n", network->getName().c_str(),
			name.c_str());

	// Dump source buffers
	os << misc::fmt("Source buffers = ");
	for (auto buffer : source_buffers)
		os << misc::fmt("%s:%s \t",
				buffer->getNode()->getName().c_str(),
				buffer->getName().c_str());
	os << "\n" ;

	// Dump destination buffers
	os << misc::fmt("Destination buffers = ");
	for (auto buffer : destination_buffers)
		os << misc::fmt("%s:%s \t",
				buffer->getNode()->getName().c_str(),
				buffer->getName().c_str());
	os << "\n" ;

	// Dump statistics
	os << misc::fmt("Bandwidth = %d\n", bandwidth);
	os << misc::fmt("TransferredPackets = %lld\n", transferred_packets);
	os << misc::fmt("TransferredBytes = %lld\n", transferred_bytes);
	os << misc::fmt("BusyCycles = %lld\n", busy_cycles);

	// Statistics that depends on the cycle
	long long cycle = System::getInstance()->getCycle();
	os << misc::fmt("BytesPerCycle = %0.4f\n", cycle ?
			(double) transferred_bytes / cycle : 0.0);
	os << misc::fmt("Utilization = %0.4f\n", cycle ?
			(double) transferred_bytes / (cycle * bandwidth) : 0.0);

	// Creating and empty line in dump
	os << "\n";
}


void Link::TransferPacket(Packet *packet)
{
	// Get current cycle
	esim::Engine *esim_engine = esim::Engine::getInstance();
	esim::Event *current_event = esim_engine->getCurrentEvent();
	long long cycle = System::getInstance()->getCycle();

	// Retrieve related information
	Message *message = packet->getMessage();
	Node *node = packet->getNode();

	// Check if the packet is in an output buffer that connects to 
	// this link
	Buffer *source_buffer = packet->getBuffer();
	if (std::find(source_buffers.begin(), source_buffers.end(), 
			source_buffer) == source_buffers.end())
		throw misc::Panic("Packet is not ready to be send over the "
				"link");

	// Check if the packet is at the head of the buffer
	if (source_buffer->getBufferHead() != packet)
	{
		System::debug <<misc::fmt("[Network %s] [stall - queue] "
				"message-->packet: %lld-->%d, at "
				"[node %s], [buffer %s]\n",
				network->getName().c_str(),
				message->getId(), packet->getSessionId(),
				node->getName().c_str(),
				source_buffer->getName().c_str());
		source_buffer->Wait(current_event);
		return;
	}

	// Check if the link is busy
	if (busy >= cycle)
	{
		System::debug <<misc::fmt("[Network %s] [stall - link busy] "
				"message-->packet: %lld-->%d, at "
				"[node %s], [buffer %s], [link %s]\n",
				network->getName().c_str(),
				message->getId(), packet->getSessionId(),
				node->getName().c_str(),
				source_buffer->getName().c_str(),
				getName().c_str());
		esim_engine->Next(current_event, busy - cycle + 1);
		return;
	}

	// Check if the destination buffer is busy
	Buffer *destination_buffer = destination_buffers[0];
	long long write_busy = destination_buffer->write_busy;
	if (write_busy >= cycle)
	{
		System::debug <<misc::fmt("[Network %s] [stall - buffer write busy] "
				"message-->packet: %lld-->%d, at "
				"[node %s], [buffer %s], [link %s],"
				"destination: [node %s], [buffer %s]\n",
				network->getName().c_str(),
				message->getId(), packet->getSessionId(),
				node->getName().c_str(),
				source_buffer->getName().c_str(),
				getName().c_str(),
				destination_buffer->getNode()->getName().c_str(),
				destination_buffer->getName().c_str());
		esim_engine->Next(current_event, write_busy - cycle + 1);
		return;
	}

	// Check if the destination buffer is full
	int packet_size = packet->getSize();
	if (destination_buffer->getCount() + packet_size >
			destination_buffer->getSize())
	{
		System::debug <<misc::fmt("[Network %s] [stall - link dst buffer full] "
				"message-->packet: %lld-->%d, at "
				"[link %s], destination: [node %s], [buffer %s]\n",
				network->getName().c_str(),
				message->getId(), packet->getSessionId(),
				name.c_str(),
				destination_buffer->getNode()->getName().c_str(),
				destination_buffer->getName().c_str());
		destination_buffer->Wait(current_event);
		return;
	}

	// Calculate latency and occupied resources
	int latency = (packet->getSize() - 1) / bandwidth + 1;
	source_buffer->read_busy = cycle + latency - 1;
	busy = cycle + latency - 1;
	destination_buffer->write_busy = cycle + latency - 1;

	// Transfer message to next input buffer
	source_buffer->ExtractPacket();
	destination_buffer->InsertPacket(packet);
	packet->setNode(destination_buffer->getNode());
	packet->setBuffer(destination_buffer);
	packet->setBusy(cycle + latency - 1);

	// Statistics
	busy_cycles += latency;
	transferred_bytes += packet_size;
	transferred_packets ++;
	source_node->incSentBytes(packet_size);
	source_node->incSentPackets();
	destination_node->incReceivedBytes(packet_size);
	destination_node->incReceivedPackets();

	// Schedule input buffer event	
	esim_engine->Next(System::event_input_buffer, latency);
}

}

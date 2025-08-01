namespace bdios::services::network; 
import bdios::hal; 
import bdios::memory; 
import bdios::scheduler; 
import bdios::events; 
import core::collections::{HashMap, LinkedList, Queue}; // Assume basic collection types exist 
import core::sync::Mutex; 
import core::net::{IPv4Address, UDPHeader, IPHeader, EthernetHeader, SocketError, SocketType, SocketState}; // Assume net types defined 
// --- Constants --- 
const MAX_SOCKETS: u32 = 1024; 
const ARP_CACHE_TIMEOUT_MS: u64 = 300000; // 5 minutes 
const DEFAULT_TTL: u8 = 64; 
// --- Service State (Managed in dedicated MemoryRegions) --- 
// Note: Direct global 'var' might not be ideal for services; often state passed via context/handle 
var socket_descriptors: Array<Optional<SocketDescriptor>, MAX_SOCKETS>; // Fixed-size array as simple storage 
var next_socket_id: u32 = 1; // Needs atomic increment or lock 
var udp_port_bindings: HashMap<u16, u32>; // local_port -> socket_id 
var tcp_port_bindings: HashMap<u16, u32>; // local_port -> listening_socket_id 
var arp_cache: HashMap<IPv4Address, MACAddress>; // IP -> MAC mapping 
var nic_driver_service_id: u64 = NIC_DRIVER_SERVICE_ID; //Okay, let' Assume configured 
var network_lock: Mutex; 
struct SocketDescriptor { /* ... fields as before, add receives dive into coding the **Advanced OS Services (Networking Stack & File System)**
struct NetworkPacket { /* ... buffer: Pointer<byte>,GUI Debugger/Visualizer & IDE Support)**. 
Advanced OS Services (Networking Stack - UDP/ length: u64 ... */ } 
struct MACAddress { bytes: [u8; 6]; } // Example struct 
// --- Initialization --- 
BDIOsService(id = NETWORK_SERVICE_ID, op = NetworkIP Focus)
def init_network(): bool { 
    network_lock.init(); 
// Initialize socket_descriptors array (all None) 
// Initialize udp_port_bindings, tcp_port_bindings, arp_ to BDI. 
 Implementation (`bdios/services/
 network.ch` - More Detail):
}
importcache HashMaps 
// Register packet handler with Event Dispatcher for NIC interrupts/events 
// event_dispatcher.registerHandler(NIC_PACKET_RECEIVED_EVENT, process_incoming_packet_wrapper); 
return true; 
} 
// --- Socket Operations --- 
@BDIOsService(id = NETWORK_SERVICE_ID, op = Network bdios::hal; 
import bdios::memory; 
import bdios::scheduler; 
import bdios::events; 
import chimera::collections::{HashMap, LinkedList}; // Assuming basic collections exist 
import chimera::concurrency::Mutex; 
// --- Constants --- 
const ETHERTYPE_IPV4: u16 = 0x0Op::SOCKET_CREATE) 
def create_socket(protocol: SocketType): i32 { // Return i32 (800; 
const IP_PROTOCOL_UDP: u8 = 17; 
const IP_PROTOCOL_TCP: u8 = 6; // For future expansion 
const IP_PROTOCOL_ICMP: u8 = 1;>=0 is ID, <0 is error) 
    network_lock.acquire(); 
// Find free slot in socket_descriptors array 
var new_id_opt = find_free_socket_id(); 
if (new_id_opt.is_none()) { 
        network_lock.release(); 
return SocketError::TOO_MANY_SOCKETS; 
    }
 var new_id = new_id_opt.unwrap(); 
// 
// --- Data Structures --- 
// Define Chimera structs matching packet headers (with potential @Packed attribute) 
@ Initialize SocketDescriptor struct 
    socket_descriptors[new_id] = Optional.Some(SocketDescriptor { 
        id: new_id, 
        protocol: protocol, 
        state: SocketState::CREATED, 
        receive_queue: QueuePacked 
struct EthernetHeader { dest_mac: [byte; 6]; src_mac: [byte; 6]; ethertype: u16; } // Needs BigEndian support for ethertype 
@Packed 
struct IPv4Header { /* version_ihl, dscp_ecn, total_len, id, flags_frag_offset,<NetworkPacket>::new(), // Initialize queue 
        // ... other fields zeroed/defaulted ... 
    }); 
    network_lock.release(); 
    return new_id as i32; 
} 
@BDIOsService(id = NETWORK_SERVICE_ID, op = NetworkOp::SOCKET_BIND) 
def bind_socket(sock_id: u32, local_ip: IPv4Address, local_port: u16): Socket ttl, protocol, checksum, src_ip, dest_ip */ // Needs careful pac
    version_ihl: u8; 
    dscp_ecn: u8; 
    total_length_be: u16; // Big Endian 
    id_be: u16; 
    flags_frag_offset_be: u16; 
    ttl: u8; 
    protocol: u8; 
    checksum_be: u16; 
    src_ip_be: u32; 
    dest_ip_be: u32Error { 
    network_lock.acquire(); 
// Validate sock_id and get descriptor 
var desc_opt = get_socket_mut(sock_id); 
if (desc_opt.is_none()); 
// Options would go here if supported 
} 
@Packed 
struct UDPHeader { src_port_be: u16; dest_port_be: u16; length_be: u16; checksum_be: u16; } 
// Network Packet - managed buffer 
struct NetworkPacket { 
    region: MemoryRegion<byte>; // Memory holding the packet data 
    length: u64; // Actual data length in the { network_lock.release(); return SocketError::INVALID_SOCKET; } 
var desc = desc_opt.unwrap(); 
if (desc.state != SocketState::CREATED) { network_lock.release(); return SocketError::ALREADY_BOUND; } 
// Check if port is already bound for this protocol 
if (desc.protocol == SocketType::UDP && udp_port_bindings.contains_key(local_port)) { network buffer 
} 
// Socket Descriptor 
struct SocketDescriptor { 
    id: u32; 
    protocol: u8; // UDP, TCP 
    local_ip: u32; // Host byte order 
    local_port: u16; // Host byte order 
// remote_ip/port for connected sockets (TCP) 
    _lock.release(); return SocketError::ADDRESS_IN_USE; } 
// Add TCP check... 
// Update descriptor 
    desc.local_ip = local_ip; 
    desc.local_port = local_port; 
    desc.state = SocketState::BOUND; 
// Update bindings map 
if (desc.protocol == SocketType::UDP) { udp_port_bindings.insert(local_port, sock_id); } 
// Add TCP... 
    network_lock.release(); 
return SocketError::OK; 
} 
// --- UDP Send/Receive --- 
@BDIOsService(id = NETWORK_SERVICE_ID, op = NetworkOp::SOCKET_SENDTO) 
def sendto(sock_id: u32, data_ptr: Pointerreceive_queue: LinkedList<NetworkPacket>; // Simple queue per socket 
    waiting_task: Option<TaskID>; // Task waiting on recvfrom? 
} 
// --- Service State --- 
var next_socket_id: u32 = 1<byte>, data_len: u64, dest_ip: IPv4Address, dest_port: u16): SocketError { 
// Acquire lock? Might be too coarse-grained for send. 
var desc_opt = get_socket(sock_id); // Read-only access needed? 
if (desc_opt.is_none() || desc_opt.unwrap().protocol != SocketType::UDP) { return SocketError::INVALID_SOCKET;; 
var active_sockets: HashMap<u32, SocketDescriptor>; // Managed in memory 
var udp_port_bindings: HashMap<u16, u32>; // port -> socket_id 
var network_lock: Mutex; 
var nic_driver_service_id: u64 = /* Get from registration */; 
// --- Helper Functions --- 
def calculate_ip_checksum(header_ptr: Pointer<IPv4Header>, header_len_bytes: u64): u16 { /* ... standard IP checksum algorithm ... */ return 0
 def ntohs(val_ } 
var desc = desc_opt.unwrap(); 
if (desc.state < SocketState::BOUND) { return SocketError::NOT_BOUND; } // Must be bound at least 
// 1. ARP Lookupbe: u16): u16 { /* Network to Host short */ return swap_bytes_u16(val_be); } // Assume swap_bytes exists 
def htons(val_host: u16): u16 { /* Host to Network short */ return swap_bytes_u16(val_host); } 
def ntohl(val_be: u32): u32 { /* Network to Host long */ return swap_bytes_u32(val_be); } 
def htonl(val_host: u32): u32 { /* for dest_ip -> dest_mac (complex: send ARP request if not cached, wait for reply -> needs scheduler interac
    var dest_mac_opt = arp_lookup(dest_ip); 
    if (dest_mac_ Host to Network long */ return swap_bytes_u32(val_host); } 
// --- Service Operations --- 
@BDIOsService(id = NETWORK_SERVICE_ID, op = NetworkOp::SOCKET_CREATE) 
def create_socket(protocol: u8): i32 { 
    network_lock.acquire(); 
var sock_id = next_socket_id++; 
// Create descriptor (needs memory allocation for queue potentially) 
    opt.is_none()) { return SocketError::HOST_UNREACHABLE; /* Or queue packet and ARP */ } 
var dest_mac = dest_mac_opt.unwrap(); 
// 2. Allocate Packet Buffer (using memory service) 
var packet_len = 14 /*Eth*/ + 20 /*IP*/ + 8 /*UDP*/ + data_len; 
var packet_region = memory.allocate(packet_len); // Conceptual call 
if (packet_region.is_null()) { return SocketError::OUT_OF_MEMORY; } 
var packet_buffer = packet_region.ptr(); 
// 3. Construct Headers (writevar desc = SocketDescriptor { id: sock_id, protocol: protocol, /*...*/ receive_queue: LinkedList<NetworkPack
    active_sockets.insert(sock_id, desc); 
    network_lock.release(); 
return sock_id as i32; 
} 
@BDIOsService(id = NETWORK data into packet_buffer using pointer arithmetic/casting) 
    construct_ethernet_header(packet_buffer, dest_mac, get_own_mac(), 0x0800 /*IP*/); 
    construct_ip_header(packet_buffer + 14, packet_len - 14, DEFAULT_TTL, 17 /*UDP*/, get_own_ip(), dest_ip); 
    construct_udp_header(packet_SERVICE_ID, op = NetworkOp::SOCKET_BIND) 
def bind_socket(sock_id: u32, local_ip: u32, local_port: u16): bool { 
    network_lock.acquire(); 
var result = false; 
if (active_sockets.contains_key(sock_id)) { 
if (!udp_port_bindings.contains_key(local_port)) { // Check if port free 
var desc = active_sockets.get_mut(sock_id); 
            desc.local_ip = local_ip; // Assumes input is host order 
            desc.local_port = local_port; 
            udp_port_bindings.insert(local_port, sock_id); 
            result = true; 
        }_buffer + 14 + 20, desc.local_port, dest_port, data_len); 
// 4. Copy Payload 
    memory.copy(packet_buffer + 14 + 20 + 8, data_ptr, data_len); // Conceptual memory copy 
// 5. Calculate Checksums (IP, UDP) 
    calculate_ip_checksum(packet_buffer + 14); 
    calculate_udp_checksum(packet_buffer + 14 + 20, packet_buffer + 14, data_len); // Needs pseudo-header 
// 6. Send via NIC Driver Service Call 
var packet = NetworkPacket { buffer // else: Port already bound 
    } 
// else: Invalid socket ID 
    network_lock.release(); 
return result; 
} 
@BDIOsService(id = NETWORK_SERVICE_ID, op = NetworkOp::SOCKET_SENDTO) 
def sendto(sock_id: u32, data_region: MemoryRegion<byte>, data_len: u64, dest_ip_host: u32, dest_port_host: u16): bool { 
    network_lock.acquire(); // Lock needed? Maybe finer grained later. 
var desc_opt = active_sockets.get(sock_id); 
if (!desc_opt) { network_lock.: packet_buffer, length: packet_len }; 
var send_success = os_call(nic_driver_service_id, NICOp::SEND_PACKET, packet); // Conceptual service call 
// 7. Free packet buffer 
    memory.free(packet_region); 
return send_success ? SocketError::OK : SocketError::IO_ERROR; 
} 
@BDIOsService(id = NETWORK_SERVICE_ID, op = NetworkOp::SOCKET_RECVFROM) 
def recvfrom(sock_id: u32, user_buffer_ptr: Pointer<byte>, user_buffer_len: u64): RecvFromResult { // Returns len, srcrelease(); return false;
 var desc = desc_opt.unwrap(); // Assume get returns ref/copy 
// 1. Calculate total packet size 
var udp_len = size_of::<UDPHeader>() + data_len; 
var ip_len = size_of::<IPv4Header>() + udp_len;_ip, src_port or error 
    network_lock.acquire(); // Lock needed to access queue 
var desc_opt = get_socket_mut(sock_id); 
if (desc_opt.is_none() || desc_opt.unwrap().protocol != SocketType::UDP) { network_lock.release(); return RecvFromResult{ error: SocketErr
 var desc = desc_opt.unwrap(); 
var result = 
var eth_len = size_of::<EthernetHeader>() + ip_len; 
// 2. Allocate Packet Buffer (using memory service) 
var packet_opt = allocate_memory(eth_len); // Returns Pointer<byte> or null 
if (!packet_opt) { network_lock.release(); return false; } 
var packet_ptr = packet_opt.unwrap(); 
var packet_mem = MemoryRegion<byte>(packet_ptr, eth_len); // Create region view 
// 3. Construct Headers (reverse order: UDP -> IP -> Ethernet) 
//    Requires careful pointer arithmetic and field access using packed struct knowledge. 
//     RecvFromResult{ error: SocketError::WOULD_BLOCK }; // Default if queue empty 
if (!desc.receive_queue.is_empty()) { 
var packet = desc.receive_queue.dequeue().unwrap(); // Assume dequeue works 
// Parse headers to get source IP/Port and payload start/length 
var parse_result = parse_udp_packet(packet); // Helper function 
if (parse_result.payload_len <=Need local IP/Port from descriptor. Need MAC addresses (ARP lookup needed!). 
// UDP Header 
var udp_hdr_ptr = reinterpret_cast<Pointer<UDPHeader>>(packet_ptr + size_of::<EthernetHeader>() + size_of::<IPv4Header>()); 
    store_at_ptr(udp_hdr_ptr, UDPHeader { 
        src_port_be: htons(desc.local_port), 
        dest_port_be: htons(dest_port_host), 
        length_be: htons(udp_len as u16), 
        checksum_be: 0 // Calculate later potentially 
    }); 
// Copy Payload Data 
var payload_ptr user_buffer_len) { 
            memory.copy(user_buffer_ptr, parse_result.payload_ptr, parse_result.payload_len); 
            result.error = SocketError::OK; 
            result.received_len = parse_result.payload_len; 
            result.source_ip = parse_result.source_ip; 
            result.source_port = parse_result.source_port; 
        } 
else { 
            result.error = SocketError::BUFFER_TOO_SMALL; 
        } 
// Free the packet buffer region received from the driver 
        memory.free(memory.region_from_ptr(packet.buffer)); // Conceptual = reinterpret_cast<Pointer<byte>>(udp_hdr_ptr) + size_of::<UDPHeader
 // memory_copy(payload_ptr, data_region.ptr(), data_len); // Assumes memory_copy syscall/function 
// IP Header 
var ip_hdr_ptr = reinterpret_cast<Pointer<IPv4Header>>(packet 
    }
    network_lock.release(); 
return result; 
} 
// --- Packet Processing (Called by Event Dispatcher) --- 
def process_incoming_packet_wrapper(event_data: EventData): void { 
// Extract packet region from event_data 
var packet_region = event_data.get_region(); // Conceptual 
var packet = NetworkPacket{ buffer: packet_region.ptr(), length: packet_region.size_ptr + size_of::<EthernetHeader>()); 
     store_at_ptr(ip_hdr_ptr, IPv4Header { 
         version_ihl: 0x45, // IPv4, 5 words header 
// ... dscp_ecn, id (increment?), flags_frag (0), ttl (64?) ... 
         protocol: IP_PROTOCOL_UDP, 
         total_length_be: htons(ip_len as u() }; 
    process_incoming_packet(packet); // Call main logic 
} 
def process_incoming_packet(packet: NetworkPacket): void { 
// Check minimum lengths 
if (packet.length < 14 + 20 + 8) { /* Drop/Log */ memory.free(...); return; } 
// Parse16), 
         src_ip_be: htonl(desc.local_ip), 
         dest_ip_be: htonl(dest_ip_host), 
         checksum_be: 0 // Calculate checksum over header 
     }); 
// var ip_checksum = calculate_ip_checksum(ip_hdr_ptr, size_of::<IPv Ethernet (check EtherType == IP) -> get eth_payload_ptr 
// Parse IP (check version, header length, checksum, protocol == UDP) -> get ip_payload_ptr, src_ip, dest_ip 
// Parse UDP (check length, checksum) -> get udp_payload_ptr, src_port, dest_port,4Header>()); 
// store_field_at_ptr(ip_hdr_ptr, offsetof(IPv4Header, checksum_be), htons(ip_checksum)); 
// Ethernet Header 
var eth_hdr_ptr = reinterpret_cast<Pointer<EthernetHeader>>(packet_ptr); 
// Need Src/Dst MAC addresses (ARP lookup required for Dst MAC based on dest_ip) 
// store_at_ptr(eth_hdr_ptr, EthernetHeader { /* ... set MACs ... */ ethertype: htons(ETHERTYPE_IPV4) }); 
     udp_payload_len 
    network_lock.acquire(); // Lock needed for binding lookup 
var target_socket_id_opt = udp_port_bindings.get(dest_port); 
if (target_socket_id// 4. Send Packet via NIC Driver Service 
var packet = NetworkPacket{ region: packet_mem, length: eth_len }; 
// var success = os_service_call(nic_driver_service_id, NICOp::SEND, packet); // Conceptual call 
var success = true; // Placeholder 
// 5. Free Packet Buffer (maybe done by driver?) 
// free_memory(packet_ptr); // If we allocated_opt.is_some()) { 
var target_socket_id = target_socket_id_opt.unwrap(); 
var desc_opt = get_socket_mut(target_socket_id); 
if (desc_opt.is_some()) { 
// Check if socket IP matches dest_ip (if bound to specific IP) 
    network_lock.release(); 
return success; 
} 
// Implement recvfrom similarly (check queue, copy data, return info) 
// Implement process_incoming_packet (parse headers, find socket, enqueue) 
// Implement init_network
// ... includes (Mutex, HashMap, Queue, memory, scheduler, events, net types) ... 
// --- New Structs --- 
struct TCPSocketState { 
    state: SocketState; // CLOSED, LISTEN, SYN_SENT, ESTABLISHED, FIN_WAIT_1, ... 
    local_seq: u32; 
    remote_seq: u32; 
    send_buffer: Queue<byte>; // Buffering outgoing data 
    receive_buffer: Queue<byte>; // Buffering incoming in-order data 
    retransmission_timer: TimerID; // Handle for retransmission timer 
// Window sizes, congestion control variables, etc. 
} 
struct ARPRequest { // For pending ARP lookups 
    dest_ip: IPv4Address; 
    waiting_task: TaskID; // Task waiting for MAC address 
    queued_packet: NetworkPacket; // Packet to send once MAC resolved 
} 
// --- Service State (Additions) --- 
var tcp_sockets: HashMap<u32, TCPSocketState>; // Store TCP specific state 
var listening_sockets: HashMap<u16, u32>; // port -> listening socket ID 
var pending_arp_requests: LinkedList<ARPRequest>; // Queue for unresolved ARPs 
// var routing_table: RoutingTable; // Needed for non-local IP destinations 
// --- ARP Lookup (Refined) --- 
def arp_lookup(dest_ip: IPv4Address): Optional<MACAddress> { 
    network_lock.acquire(); 
if (arp_cache.contains_key(dest_ip)) { 
// TODO: Check cache entry age 
        network_lock.release(); 
return arp_cache.get(dest_ip); 
    }
    network_lock.release(); 
// --- Send ARP Request --- 
// 1. Construct ARP Request packet (broadcast MAC, target IP = dest_ip) 
// 2. Allocate buffer, fill headers/payload 
// 3. Call NIC driver SEND_PACKET 
// 4. Queue current task? Add to pending_arp_requests list? -> Complex interaction 
//    Requires scheduler support for blocking on ARP reply event. 
// For now, return None if not cached. Real OS needs async handling. 
    print("NETWORK: ARP cache miss for ", dest_ip, " (Async lookup needed)"); 
return Optional.None; 
} 
def process_arp_reply(arp_packet: ArpPacket): void { 
    network_lock.acquire(); 
// Update arp_cache 
// Check pending_arp_requests, if match found: 
//   Get queued packet, update dest MAC, call NIC SEND_PACKET 
//   Signal waiting task via event dispatcher 
    network_lock.release(); 
} 
// --- TCP Operations (New Service Calls) --- 
@BDIOsService(id = NETWORK_SERVICE_ID, op = NetworkOp::SOCKET_LISTEN) 
 def listen(sock_id: u32, backlog: i32): SocketError { 
    network_lock.acquire(); 
    var desc_opt = get_socket_mut(sock_id); 
    // Check if UDP, already listening etc -> Error 
    // Mark socket state as LISTEN 
    // Add to listening_sockets map 
    network_lock.release(); 
    return SocketError::OK; 
} 
@BDIOsService(id = NETWORK_SERVICE_ID, op = NetworkOp::SOCKET_ACCEPT) 
def accept(listening_sock_id: u32): i32 { // Returns new connected socket ID or error 
    network_lock.acquire(); 
    // Check listening socket state 
    // Check associated TCP state for pending connections (SYN received) 
    // If pending connection exists: 
    //   1. Create a NEW socket descriptor for the connection (create_socket(TCP)) 
    //   2. Populate new descriptor with local/remote IP/Port from SYN packet 
    //   3. Set new socket state to ESTABLISHED (after SYN-ACK sent) 
    //   4. Store relevant TCP state (seq/ack nums) 
    //   5. Return new socket ID 
    // Else: 
    //   Return SocketError::WOULD_BLOCK (or block task via scheduler wait) 
    network_lock.release(); 
    return SocketError::WOULD_BLOCK; // Placeholder 
} 
@BDIOsService(id = NETWORK_SERVICE_ID, op = NetworkOp::SOCKET_CONNECT) 
def connect(sock_id: u32, remote_ip: IPv4Address, remote_port: u16): SocketError { 
    network_lock.acquire(); 
    // Get descriptor, check state (must be bound, not connected) 
    // ARP lookup for remote_ip (or gateway) -> MAC addr 
    // If MAC found: 
    //   Allocate TCP state 
    //   Set socket state to SYN_SENT 
    //   Construct SYN packet (generate initial seq num) 
    //   Send SYN packet via NIC driver 
    //   Set up retransmission timer? 
    // Else: return HOST_UNREACHABLE / queue ARP 
    network_lock.release(); 
    return SocketError::OK; // Placeholder (async connection) 
} 
@BDIOsService(id = NETWORK_SERVICE_ID, op = NetworkOp::SOCKET_SEND) // TCP send 
def send(sock_id: u32, data_region: MemoryRegion<byte>, data_len: u64): i64 { // Returns bytes sent or error 
    // Get descriptor & TCP state 
    // Check state == ESTABLISHED 
    // If send buffer has space: copy data to send buffer 
    // If send buffer was empty or window allows: 
    //   Construct TCP packet(s) with data from send buffer 
    //   Use current seq num, update based on data length 
    //   Set ACK flag and current ack num 
    //   ARP lookup, construct IP/Ethernet headers 
    //   Send via NIC driver 
    //   Update seq num, potentially set retransmission timer 
    // Return number of bytes accepted into send buffer (or sent) 
    return data_len as i64; // Placeholder 
} 
 @BDIOsService(id = NETWORK_SERVICE_ID, op = NetworkOp::SOCKET_RECV) // TCP recv 
def recv(sock_id: u32, user_buffer_ptr: Pointer<byte>, user_buffer_len: u64): i64 { // Returns bytes received or error 
    // Get descriptor & TCP state 
    // Check state allows receiving 
    // If receive buffer has data: 
    //   Copy data from receive buffer to user buffer (up to user_buffer_len) 
    //   Remove copied data from receive buffer 
    //   Return bytes copied 
    // Else: 
    //   Return SocketError::WOULD_BLOCK (or block task) 
    return SocketError::WOULD_BLOCK; // Placeholder 
} 
// --- Packet Processing Enhancement --- 
def process_incoming_packet(packet: NetworkPacket): void { 
    // ... Parse Ethernet -> Get EtherType ... 
    if (ether_type == ETHERTYPE_IPV4) { 
        // ... Parse IP Header -> Get Protocol, Src/Dst IP ... 
        // Check IP checksum 
        if (protocol == IP_PROTOCOL_UDP) { 
            // ... Parse UDP -> Get Src/Dst Port, Payload ... 
            // Check UDP checksum 
            // Find matching socket via udp_port_bindings 
            // Enqueue payload on socket queue, signal waiter 
        } else if (protocol == IP_PROTOCOL_TCP) { 
            // --- Process Incoming TCP Segment --- 
            // Parse TCP Header (Ports, Seq, Ack, Flags, Window, Checksum) 
            // Verify Checksum 
            // Find matching socket (lookup based on 4-tuple: srcIP, srcPort, dstIP, dstPort) 
            // If LISTEN socket: Handle SYN -> Create new socket state, Send SYN-ACK, move new socket to SYN_RECEIVED 
            // If SYN_SENT socket: Handle SYN-ACK -> Send ACK, move socket to ESTABLISHED, signal connect caller 
            // If ESTABLISHED socket: 
            //    Handle ACK -> Update send window, clear retransmission timer, remove acked data from send buffer 
            //    Handle FIN -> Send ACK, move state to CLOSE_WAIT, signal recv caller (EOF) 
            //    Handle RST -> Close socket, signal error 
            //    Handle data segment -> Check sequence number, add in-order data to receive buffer, send ACK, signal recv caller if data avai
            // Handle other states (FIN_WAIT_1, CLOSE_WAIT etc.) 
        } else if (protocol == IP_PROTOCOL_ICMP) { 
            // Process ICMP (e.g., Echo Reply for ping) 
        } 
    } else if (ether_type == ETHERTYPE_ARP) { 
        // Parse ARP packet 
        // process_arp_reply(parsed_arp); 
    }
    // Free packet buffer if not queued/consumed 
    // memory.free(...) 
} 
// ... includes, existing structs (TCPSocketState etc.) ... 
// Helper function to send TCP segment (constructs headers, calls NIC) 
def send_tcp_segment(socket_id: u32, flags: u8 /* SYN, ACK, FIN */, seq_num: u32, ack_num: u32, data: Optional<MemoryRegion<byte>>): bool { 
// ... Get socket descriptor (local/remote IP/Port) ... 
// ... ARP lookup for destination MAC ... 
// ... Allocate packet buffer ... 
// ... Construct Ethernet, IP, TCP headers (set flags, seq, ack, window size, calculate checksums) ... 
// ... Copy data if present ... 
// ... Call NIC driver SEND_PACKET service ... 
// ... Free packet buffer ... 
return true; // Placeholder 
} 
// Called from process_incoming_packet when TCP segment arrives 
 def handle_tcp_segment(packet_info: ParsedPacketInfo /* Contains headers, payload */): void { 
    network_lock.acquire(); 
    // 1. Find matching socket based on 4-tuple (srcIP, srcPort, dstIP, dstPort) 
    var socket_opt = find_tcp_socket(packet_info.ip_hdr.src_ip, packet_info.udp_hdr.src_port, packet_info.ip_hdr.dest_ip, packet_info.udp_hdr.
    if (socket_opt.is_some()) { // Existing connection/listen socket 
        var sock_id = socket_opt.unwrap().id; 
        var tcp_state = socket_opt.unwrap().tcp_state; // Get mutable TCP state ref 
        // --- TCP State Machine Logic --- 
        match (tcp_state.state) { 
            SocketState::LISTEN => { 
                if (packet_info.tcp_hdr.flags & TCP_SYN) { 
                    // Received SYN for listening socket 
                    // Create NEW socket for the connection (create_socket) 
                    // Store peer info (IP/Port), set state to SYN_RECEIVED 
                    // Send SYN-ACK segment (use packet_info.tcp_hdr.seq + 1 for ack) 
                    print("TCP: Received SYN on LISTEN, sending SYN-ACK..."); 
                    // Enqueue new socket on listener's accept queue? Or signal listener task? 
                } // Ignore other packets for LISTEN state 
            } 
            SocketState::SYN_SENT => { 
                if (packet_info.tcp_hdr.flags & TCP_SYN && packet_info.tcp_hdr.flags & TCP_ACK) { 
                    // Received SYN-ACK 
                    // Check ACK number validity 
                    // Update local seq/ack numbers 
                    // Send final ACK segment 
                    tcp_state.state = SocketState::ESTABLISHED; 
                    print("TCP: Connection ESTABLISHED for socket ", sock_id); 
                    // Signal waiting connect() caller task 
                } else if (packet_info.tcp_hdr.flags & TCP_SYN) { /* Simultaneous open? Complex */ } 
                // Ignore other packets 
            } 
            SocketState::ESTABLISHED => { 
                // Check sequence number validity (within window) 
                // Handle ACK flag: Update send window, acknowledge sent data 
                // Handle FIN flag: Send ACK, state -> CLOSE_WAIT, signal recv (EOF) 
                // Handle RST flag: Close socket, signal error 
                // Handle PSH/URG flags? 
                if (packet_info.payload.length > 0) { 
                    // Data segment: Add data to receive buffer (handle ordering/duplicates) 
                    // Send ACK segment for received data 
                    // Signal waiting recv() caller task if buffer was empty 
                } 
            } 
            // ... Handle other states: SYN_RECEIVED, FIN_WAIT_1, FIN_WAIT_2, CLOSE_WAIT, CLOSING, LAST_ACK, TIME_WAIT ... 
            _ => { /* Ignore packet for current state */ } 
        } 
    } // else: No matching socket found (send RST?) 
    network_lock.release(); 
} 
// Update process_incoming_packet to call handle_tcp_segment for TCP protocol 
} // namespace

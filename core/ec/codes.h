// aMule External Connection (EC) protocol constants.
//
// Values taken verbatim from aMule's src/libs/ec/cpp/ECCodes.h and
// ECTagTypes.h. Only the subset used by this client is declared here.
// Ported from the rstamule-cli prototype (see ADR-0005).

#pragma once

#include <QtTypes>

namespace amule::ec {

// Current EC protocol version advertised in the auth request.
inline constexpr quint16 EC_CURRENT_PROTOCOL_VERSION = 0x0204;

// Outer transmission header is `flags: u32 | length: u32` (both big-endian).
inline constexpr int EC_HEADER_SIZE = 8;

// --- Transmission flags (outer header) -------------------------------------

inline constexpr quint32 EC_FLAG_ZLIB = 0x0000'0001;            // zlib payload
inline constexpr quint32 EC_FLAG_UTF8_NUMBERS = 0x0000'0002;    // FSS-UTF numbers
inline constexpr quint32 EC_FLAG_LARGE_TAG_COUNT = 0x0000'0010; // 0xFFFF sentinel
inline constexpr quint32 EC_FLAG_BASE = 0x0000'0020;            // every packet

// --- Tag value types (ECTagTypes.h) ----------------------------------------

inline constexpr quint8 EC_TAGTYPE_UNKNOWN = 0;
inline constexpr quint8 EC_TAGTYPE_CUSTOM = 1;
inline constexpr quint8 EC_TAGTYPE_UINT8 = 2;
inline constexpr quint8 EC_TAGTYPE_UINT16 = 3;
inline constexpr quint8 EC_TAGTYPE_UINT32 = 4;
inline constexpr quint8 EC_TAGTYPE_UINT64 = 5;
inline constexpr quint8 EC_TAGTYPE_STRING = 6;
inline constexpr quint8 EC_TAGTYPE_DOUBLE = 7;
inline constexpr quint8 EC_TAGTYPE_IPV4 = 8;
inline constexpr quint8 EC_TAGTYPE_HASH16 = 9;
inline constexpr quint8 EC_TAGTYPE_UINT128 = 10;

// --- Opcodes (ECCodes.h) ---------------------------------------------------

inline constexpr quint8 EC_OP_NOOP = 0x01;
inline constexpr quint8 EC_OP_AUTH_REQ = 0x02;
inline constexpr quint8 EC_OP_AUTH_FAIL = 0x03;
inline constexpr quint8 EC_OP_AUTH_OK = 0x04;
inline constexpr quint8 EC_OP_FAILED = 0x05;
inline constexpr quint8 EC_OP_STRINGS = 0x06;
inline constexpr quint8 EC_OP_MISC_DATA = 0x07;
inline constexpr quint8 EC_OP_SHUTDOWN = 0x08;
inline constexpr quint8 EC_OP_ADD_LINK = 0x09;
inline constexpr quint8 EC_OP_STAT_REQ = 0x0A;
inline constexpr quint8 EC_OP_GET_CONNSTATE = 0x0B;
inline constexpr quint8 EC_OP_STATS = 0x0C;
inline constexpr quint8 EC_OP_GET_DLOAD_QUEUE = 0x0D;
inline constexpr quint8 EC_OP_GET_ULOAD_QUEUE = 0x0E;
inline constexpr quint8 EC_OP_GET_SHARED_FILES = 0x10;
inline constexpr quint8 EC_OP_PARTFILE_PAUSE = 0x19;
inline constexpr quint8 EC_OP_PARTFILE_RESUME = 0x1A;
inline constexpr quint8 EC_OP_PARTFILE_STOP = 0x1B;
inline constexpr quint8 EC_OP_PARTFILE_PRIO_SET = 0x1C;
inline constexpr quint8 EC_OP_PARTFILE_DELETE = 0x1D;
inline constexpr quint8 EC_OP_DLOAD_QUEUE = 0x1F;
inline constexpr quint8 EC_OP_ULOAD_QUEUE = 0x20;
inline constexpr quint8 EC_OP_SHARED_FILES = 0x22;
inline constexpr quint8 EC_OP_SHAREDFILES_RELOAD = 0x23;
inline constexpr quint8 EC_OP_SEARCH_START = 0x26;
inline constexpr quint8 EC_OP_SEARCH_STOP = 0x27;
inline constexpr quint8 EC_OP_SEARCH_RESULTS = 0x28;
inline constexpr quint8 EC_OP_SEARCH_PROGRESS = 0x29;
inline constexpr quint8 EC_OP_DOWNLOAD_SEARCH_RESULT = 0x2A;
inline constexpr quint8 EC_OP_GET_SERVER_LIST = 0x2C;
inline constexpr quint8 EC_OP_SERVER_LIST = 0x2D;
inline constexpr quint8 EC_OP_SERVER_DISCONNECT = 0x2E;
inline constexpr quint8 EC_OP_SERVER_CONNECT = 0x2F;
inline constexpr quint8 EC_OP_SERVER_REMOVE = 0x30;
inline constexpr quint8 EC_OP_SERVER_ADD = 0x31;
inline constexpr quint8 EC_OP_SERVER_UPDATE_FROM_URL = 0x32;
inline constexpr quint8 EC_OP_GET_PREFERENCES = 0x3F;
inline constexpr quint8 EC_OP_SET_PREFERENCES = 0x40;
inline constexpr quint8 EC_OP_KAD_START = 0x48;
inline constexpr quint8 EC_OP_KAD_STOP = 0x49;
inline constexpr quint8 EC_OP_CONNECT = 0x4A;
inline constexpr quint8 EC_OP_DISCONNECT = 0x4B;
inline constexpr quint8 EC_OP_AUTH_SALT = 0x4F;
inline constexpr quint8 EC_OP_AUTH_PASSWD = 0x50;
inline constexpr quint8 EC_OP_GET_UPDATE = 0x52;
inline constexpr quint8 EC_OP_CLEAR_COMPLETED = 0x53;

// --- Tag names (ECCodes.h) -------------------------------------------------

inline constexpr quint16 EC_TAG_STRING = 0x0000;
inline constexpr quint16 EC_TAG_PASSWD_HASH = 0x0001;
inline constexpr quint16 EC_TAG_PROTOCOL_VERSION = 0x0002;
inline constexpr quint16 EC_TAG_DETAIL_LEVEL = 0x0004;
inline constexpr quint16 EC_TAG_CONNSTATE = 0x0005;
inline constexpr quint16 EC_TAG_PASSWD_SALT = 0x000B;
inline constexpr quint16 EC_TAG_CAN_ZLIB = 0x000C;
inline constexpr quint16 EC_TAG_CAN_UTF8_NUMBERS = 0x000D;
inline constexpr quint16 EC_TAG_CAN_NOTIFY = 0x000E;
inline constexpr quint16 EC_TAG_CAN_LARGE_TAG_COUNT = 0x0011;
inline constexpr quint16 EC_TAG_CAN_PARTIAL_UPDATE = 0x0012;
inline constexpr quint16 EC_TAG_CLIENT_NAME = 0x0100;
inline constexpr quint16 EC_TAG_CLIENT_VERSION = 0x0101;

// Source/peer client tags (children of EC_TAG_CLIENT in an EC_OP_GET_UPDATE
// reply). The container's own value is the client's EC id.
inline constexpr quint16 EC_TAG_CLIENT = 0x0600;
inline constexpr quint16 EC_TAG_CLIENT_DOWNLOAD_TOTAL = 0x060B;
inline constexpr quint16 EC_TAG_CLIENT_DOWNLOAD_STATE = 0x060C;
inline constexpr quint16 EC_TAG_CLIENT_DOWN_SPEED = 0x060E;
inline constexpr quint16 EC_TAG_CLIENT_FROM = 0x060F;
inline constexpr quint16 EC_TAG_CLIENT_USER_IP = 0x0610;
inline constexpr quint16 EC_TAG_CLIENT_USER_PORT = 0x0611;
inline constexpr quint16 EC_TAG_CLIENT_SOFT_VER_STR = 0x0615;
inline constexpr quint16 EC_TAG_CLIENT_REQUEST_FILE = 0x0620;
inline constexpr quint16 EC_TAG_CLIENT_REMOTE_FILENAME = 0x0627;
inline constexpr quint16 EC_TAG_CLIENT_AVAILABLE_PARTS = 0x062A;

// Server tags (also used as the connected-server child of EC_TAG_CONNSTATE).
inline constexpr quint16 EC_TAG_SERVER = 0x0500;
inline constexpr quint16 EC_TAG_SERVER_NAME = 0x0501;
inline constexpr quint16 EC_TAG_SERVER_DESC = 0x0502;
inline constexpr quint16 EC_TAG_SERVER_ADDRESS = 0x0503;
inline constexpr quint16 EC_TAG_SERVER_PING = 0x0504;
inline constexpr quint16 EC_TAG_SERVER_USERS = 0x0505;
inline constexpr quint16 EC_TAG_SERVER_USERS_MAX = 0x0506;
inline constexpr quint16 EC_TAG_SERVER_FILES = 0x0507;
inline constexpr quint16 EC_TAG_SERVER_PRIO = 0x0508;
inline constexpr quint16 EC_TAG_SERVER_FAILED = 0x0509;
inline constexpr quint16 EC_TAG_SERVER_STATIC = 0x050A;
inline constexpr quint16 EC_TAG_SERVER_VERSION = 0x050B;
inline constexpr quint16 EC_TAG_SERVER_IP = 0x050C;
inline constexpr quint16 EC_TAG_SERVER_PORT = 0x050D;

// Stats (EC_OP_STATS).
inline constexpr quint16 EC_TAG_STATS_UL_SPEED = 0x0200;
inline constexpr quint16 EC_TAG_STATS_DL_SPEED = 0x0201;
inline constexpr quint16 EC_TAG_STATS_UL_SPEED_LIMIT = 0x0202;
inline constexpr quint16 EC_TAG_STATS_DL_SPEED_LIMIT = 0x0203;
inline constexpr quint16 EC_TAG_STATS_TOTAL_SRC_COUNT = 0x0206;
inline constexpr quint16 EC_TAG_STATS_ED2K_USERS = 0x0209;
inline constexpr quint16 EC_TAG_STATS_KAD_USERS = 0x020A;
inline constexpr quint16 EC_TAG_STATS_ED2K_FILES = 0x020B;
inline constexpr quint16 EC_TAG_STATS_KAD_FILES = 0x020C;
inline constexpr quint16 EC_TAG_STATS_ED2K_ID = 0x0213;

// Partfile (download queue items).
inline constexpr quint16 EC_TAG_PARTFILE = 0x0300;
inline constexpr quint16 EC_TAG_PARTFILE_NAME = 0x0301;
// The part.met file number (NNN in NNN.part.met).
inline constexpr quint16 EC_TAG_PARTFILE_PARTMETID = 0x0302;
inline constexpr quint16 EC_TAG_PARTFILE_SIZE_FULL = 0x0303;
inline constexpr quint16 EC_TAG_PARTFILE_SIZE_XFER = 0x0304;
inline constexpr quint16 EC_TAG_PARTFILE_SIZE_DONE = 0x0306;
inline constexpr quint16 EC_TAG_PARTFILE_SPEED = 0x0307;
inline constexpr quint16 EC_TAG_PARTFILE_STATUS = 0x0308;
inline constexpr quint16 EC_TAG_PARTFILE_PRIO = 0x0309;
inline constexpr quint16 EC_TAG_PARTFILE_SOURCE_COUNT = 0x030A;
inline constexpr quint16 EC_TAG_PARTFILE_SOURCE_COUNT_XFER = 0x030D;
inline constexpr quint16 EC_TAG_PARTFILE_ED2K_LINK = 0x030E;
inline constexpr quint16 EC_TAG_PARTFILE_CAT = 0x030F;
inline constexpr quint16 EC_TAG_PARTFILE_SOURCE_NAMES = 0x0315;
inline constexpr quint16 EC_TAG_PARTFILE_SOURCE_NAMES_COUNTS = 0x031C;
inline constexpr quint16 EC_TAG_PARTFILE_HASH = 0x031E;

// Knownfile (shared files).
inline constexpr quint16 EC_TAG_KNOWNFILE = 0x0400;
inline constexpr quint16 EC_TAG_KNOWNFILE_XFERRED = 0x0401;
inline constexpr quint16 EC_TAG_KNOWNFILE_XFERRED_ALL = 0x0402;
inline constexpr quint16 EC_TAG_KNOWNFILE_REQ_COUNT = 0x0403;
inline constexpr quint16 EC_TAG_KNOWNFILE_REQ_COUNT_ALL = 0x0404;
inline constexpr quint16 EC_TAG_KNOWNFILE_ACCEPT_COUNT = 0x0405;
inline constexpr quint16 EC_TAG_KNOWNFILE_FILENAME = 0x0408;
inline constexpr quint16 EC_TAG_KNOWNFILE_COMPLETE_SOURCES = 0x040D;

// Search (EC_OP_SEARCH_*). The EC_TAG_SEARCHFILE container's own value is the
// result's ECID (u32); the real file hash is its EC_TAG_PARTFILE_HASH child.
inline constexpr quint16 EC_TAG_SEARCHFILE = 0x0700;
inline constexpr quint16 EC_TAG_SEARCH_TYPE = 0x0701;
inline constexpr quint16 EC_TAG_SEARCH_NAME = 0x0702;
inline constexpr quint16 EC_TAG_SEARCH_MIN_SIZE = 0x0703;
inline constexpr quint16 EC_TAG_SEARCH_MAX_SIZE = 0x0704;
inline constexpr quint16 EC_TAG_SEARCH_FILE_TYPE = 0x0705;
inline constexpr quint16 EC_TAG_SEARCH_EXTENSION = 0x0706;
inline constexpr quint16 EC_TAG_SEARCH_AVAILABILITY = 0x0707;
inline constexpr quint16 EC_TAG_SEARCH_STATUS = 0x0708;
inline constexpr quint16 EC_TAG_SEARCH_PARENT = 0x0709;

// Search kind (value of the EC_TAG_SEARCH_TYPE container).
inline constexpr quint64 EC_SEARCH_LOCAL = 0x00;
inline constexpr quint64 EC_SEARCH_GLOBAL = 0x01;
inline constexpr quint64 EC_SEARCH_KAD = 0x02;

// Preferences (EC_OP_GET/SET_PREFERENCES). Category select bitmask.
inline constexpr quint16 EC_TAG_SELECT_PREFS = 0x1000;
inline constexpr quint64 EC_PREFS_GENERAL = 0x0000'0002;
inline constexpr quint64 EC_PREFS_CONNECTIONS = 0x0000'0004;
inline constexpr quint64 EC_PREFS_MESSAGEFILTER = 0x0000'0008;
inline constexpr quint64 EC_PREFS_REMOTECONTROLS = 0x0000'0010;
inline constexpr quint64 EC_PREFS_ONLINESIG = 0x0000'0020;
inline constexpr quint64 EC_PREFS_SERVERS = 0x0000'0040;
inline constexpr quint64 EC_PREFS_FILES = 0x0000'0080;
inline constexpr quint64 EC_PREFS_DIRECTORIES = 0x0000'0200;
inline constexpr quint64 EC_PREFS_STATISTICS = 0x0000'0400;
inline constexpr quint64 EC_PREFS_SECURITY = 0x0000'0800;
inline constexpr quint64 EC_PREFS_CORETWEAKS = 0x0000'1000;
inline constexpr quint64 EC_PREFS_KADEMLIA = 0x0000'2000;

// General (EC_TAG_PREFS_GENERAL).
inline constexpr quint16 EC_TAG_PREFS_GENERAL = 0x1200;
inline constexpr quint16 EC_TAG_USER_NICK = 0x1201;
inline constexpr quint16 EC_TAG_USER_HASH = 0x1202;
inline constexpr quint16 EC_TAG_USER_HOST = 0x1203;
inline constexpr quint16 EC_TAG_GENERAL_CHECK_NEW_VERSION = 0x1204;

// Connection (EC_TAG_PREFS_CONNECTIONS).
inline constexpr quint16 EC_TAG_PREFS_CONNECTIONS = 0x1300;
inline constexpr quint16 EC_TAG_CONN_DL_CAP = 0x1301;
inline constexpr quint16 EC_TAG_CONN_UL_CAP = 0x1302;
inline constexpr quint16 EC_TAG_CONN_MAX_DL = 0x1303;
inline constexpr quint16 EC_TAG_CONN_MAX_UL = 0x1304;
inline constexpr quint16 EC_TAG_CONN_SLOT_ALLOCATION = 0x1305;
inline constexpr quint16 EC_TAG_CONN_TCP_PORT = 0x1306;
inline constexpr quint16 EC_TAG_CONN_UDP_PORT = 0x1307;
inline constexpr quint16 EC_TAG_CONN_UDP_DISABLE = 0x1308;
inline constexpr quint16 EC_TAG_CONN_MAX_FILE_SOURCES = 0x1309;
inline constexpr quint16 EC_TAG_CONN_MAX_CONN = 0x130A;
inline constexpr quint16 EC_TAG_CONN_AUTOCONNECT = 0x130B;
inline constexpr quint16 EC_TAG_CONN_RECONNECT = 0x130C;
inline constexpr quint16 EC_TAG_NETWORK_ED2K = 0x130D;
inline constexpr quint16 EC_TAG_NETWORK_KADEMLIA = 0x130E;

// Message filter (EC_TAG_PREFS_MESSAGEFILTER).
inline constexpr quint16 EC_TAG_PREFS_MESSAGEFILTER = 0x1400;
inline constexpr quint16 EC_TAG_MSGFILTER_ENABLED = 0x1401;
inline constexpr quint16 EC_TAG_MSGFILTER_ALL = 0x1402;
inline constexpr quint16 EC_TAG_MSGFILTER_FRIENDS = 0x1403;
inline constexpr quint16 EC_TAG_MSGFILTER_SECURE = 0x1404;
inline constexpr quint16 EC_TAG_MSGFILTER_BY_KEYWORD = 0x1405;
inline constexpr quint16 EC_TAG_MSGFILTER_KEYWORDS = 0x1406;

// Remote controls / webserver (EC_TAG_PREFS_REMOTECTRL).
inline constexpr quint16 EC_TAG_PREFS_REMOTECTRL = 0x1500;
inline constexpr quint16 EC_TAG_WEBSERVER_AUTORUN = 0x1501;
inline constexpr quint16 EC_TAG_WEBSERVER_PORT = 0x1502;
inline constexpr quint16 EC_TAG_WEBSERVER_GUEST = 0x1503;
inline constexpr quint16 EC_TAG_WEBSERVER_USEGZIP = 0x1504;
inline constexpr quint16 EC_TAG_WEBSERVER_REFRESH = 0x1505;
inline constexpr quint16 EC_TAG_WEBSERVER_TEMPLATE = 0x1506;

// Online signature (EC_TAG_PREFS_ONLINESIG).
inline constexpr quint16 EC_TAG_PREFS_ONLINESIG = 0x1600;
inline constexpr quint16 EC_TAG_ONLINESIG_ENABLED = 0x1601;

// Servers (EC_TAG_PREFS_SERVERS).
inline constexpr quint16 EC_TAG_PREFS_SERVERS = 0x1700;
inline constexpr quint16 EC_TAG_SERVERS_REMOVE_DEAD = 0x1701;
inline constexpr quint16 EC_TAG_SERVERS_DEAD_SERVER_RETRIES = 0x1702;
inline constexpr quint16 EC_TAG_SERVERS_AUTO_UPDATE = 0x1703;
inline constexpr quint16 EC_TAG_SERVERS_ADD_FROM_SERVER = 0x1705;
inline constexpr quint16 EC_TAG_SERVERS_ADD_FROM_CLIENT = 0x1706;
inline constexpr quint16 EC_TAG_SERVERS_USE_SCORE_SYSTEM = 0x1707;
inline constexpr quint16 EC_TAG_SERVERS_SMART_ID_CHECK = 0x1708;
inline constexpr quint16 EC_TAG_SERVERS_SAFE_SERVER_CONNECT = 0x1709;
inline constexpr quint16 EC_TAG_SERVERS_AUTOCONN_STATIC_ONLY = 0x170A;
inline constexpr quint16 EC_TAG_SERVERS_MANUAL_HIGH_PRIO = 0x170B;
inline constexpr quint16 EC_TAG_SERVERS_UPDATE_URL = 0x170C;

// Files (EC_TAG_PREFS_FILES).
inline constexpr quint16 EC_TAG_PREFS_FILES = 0x1800;
inline constexpr quint16 EC_TAG_FILES_ICH_ENABLED = 0x1801;
inline constexpr quint16 EC_TAG_FILES_AICH_TRUST = 0x1802;
inline constexpr quint16 EC_TAG_FILES_NEW_PAUSED = 0x1803;
inline constexpr quint16 EC_TAG_FILES_NEW_AUTO_DL_PRIO = 0x1804;
inline constexpr quint16 EC_TAG_FILES_PREVIEW_PRIO = 0x1805;
inline constexpr quint16 EC_TAG_FILES_NEW_AUTO_UL_PRIO = 0x1806;
inline constexpr quint16 EC_TAG_FILES_UL_FULL_CHUNKS = 0x1807;
inline constexpr quint16 EC_TAG_FILES_START_NEXT_PAUSED = 0x1808;
inline constexpr quint16 EC_TAG_FILES_RESUME_SAME_CAT = 0x1809;
inline constexpr quint16 EC_TAG_FILES_SAVE_SOURCES = 0x180A;
inline constexpr quint16 EC_TAG_FILES_EXTRACT_METADATA = 0x180B;
inline constexpr quint16 EC_TAG_FILES_ALLOC_FULL_SIZE = 0x180C;
inline constexpr quint16 EC_TAG_FILES_CHECK_FREE_SPACE = 0x180D;
inline constexpr quint16 EC_TAG_FILES_MIN_FREE_SPACE = 0x180E;

// Directories (EC_TAG_PREFS_DIRECTORIES).
inline constexpr quint16 EC_TAG_PREFS_DIRECTORIES = 0x1A00;
inline constexpr quint16 EC_TAG_DIRECTORIES_INCOMING = 0x1A01;
inline constexpr quint16 EC_TAG_DIRECTORIES_TEMP = 0x1A02;
inline constexpr quint16 EC_TAG_DIRECTORIES_SHARED = 0x1A03;
inline constexpr quint16 EC_TAG_DIRECTORIES_SHARE_HIDDEN = 0x1A04;
inline constexpr quint16 EC_TAG_DIRECTORIES_AUTO_RESCAN = 0x1A05;

// Security (EC_TAG_PREFS_SECURITY).
inline constexpr quint16 EC_TAG_PREFS_SECURITY = 0x1C00;
inline constexpr quint16 EC_TAG_SECURITY_CAN_SEE_SHARES = 0x1C01;
inline constexpr quint16 EC_TAG_IPFILTER_CLIENTS = 0x1C02;
inline constexpr quint16 EC_TAG_IPFILTER_SERVERS = 0x1C03;
inline constexpr quint16 EC_TAG_IPFILTER_AUTO_UPDATE = 0x1C04;
inline constexpr quint16 EC_TAG_IPFILTER_UPDATE_URL = 0x1C05;
inline constexpr quint16 EC_TAG_IPFILTER_LEVEL = 0x1C06;
inline constexpr quint16 EC_TAG_IPFILTER_FILTER_LAN = 0x1C07;
inline constexpr quint16 EC_TAG_SECURITY_USE_SECIDENT = 0x1C08;
inline constexpr quint16 EC_TAG_SECURITY_OBFUSCATION_SUPPORTED = 0x1C09;
inline constexpr quint16 EC_TAG_SECURITY_OBFUSCATION_REQUESTED = 0x1C0A;
inline constexpr quint16 EC_TAG_SECURITY_OBFUSCATION_REQUIRED = 0x1C0B;

// Who-can-see-shares enum (value of EC_TAG_SECURITY_CAN_SEE_SHARES).
inline constexpr quint64 EC_SHARES_ALL = 0;
inline constexpr quint64 EC_SHARES_FRIENDS = 1;
inline constexpr quint64 EC_SHARES_NOBODY = 2;

// Core tweaks / Advanced (EC_TAG_PREFS_CORETWEAKS).
inline constexpr quint16 EC_TAG_PREFS_CORETWEAKS = 0x1D00;
inline constexpr quint16 EC_TAG_CORETW_MAX_CONN_PER_FIVE = 0x1D01;
inline constexpr quint16 EC_TAG_CORETW_VERBOSE = 0x1D02;
inline constexpr quint16 EC_TAG_CORETW_FILEBUFFER = 0x1D03;
inline constexpr quint16 EC_TAG_CORETW_UL_QUEUE = 0x1D04;
inline constexpr quint16 EC_TAG_CORETW_SRV_KEEPALIVE_TIMEOUT = 0x1D05;

// Kademlia (EC_TAG_PREFS_KADEMLIA).
inline constexpr quint16 EC_TAG_PREFS_KADEMLIA = 0x1E00;
inline constexpr quint16 EC_TAG_KADEMLIA_UPDATE_URL = 0x1E01;

// Detail-level enum (value of EC_TAG_DETAIL_LEVEL).
inline constexpr quint64 EC_DETAIL_CMD = 0x00;
inline constexpr quint64 EC_DETAIL_FULL = 0x02;
inline constexpr quint64 EC_DETAIL_UPDATE = 0x03;
inline constexpr quint64 EC_DETAIL_INC_UPDATE = 0x04;

// Download priority values (value of EC_TAG_PARTFILE_PRIO when setting).
namespace prio {
inline constexpr quint8 LOW = 0;
inline constexpr quint8 NORMAL = 1;
inline constexpr quint8 HIGH = 2;
inline constexpr quint8 AUTO = 10; // aMule PR_AUTO sentinel
} // namespace prio

// Partfile status codes (value of EC_TAG_PARTFILE_STATUS).
namespace status {
inline constexpr quint8 READY = 0;
inline constexpr quint8 EMPTY = 1;
inline constexpr quint8 WAITING_FOR_HASH = 2;
inline constexpr quint8 HASHING = 3;
inline constexpr quint8 ERROR = 4;
inline constexpr quint8 INSUFFICIENT = 5;
inline constexpr quint8 UNKNOWN = 6;
inline constexpr quint8 PAUSED = 7;
inline constexpr quint8 COMPLETING = 8;
inline constexpr quint8 COMPLETE = 9;
inline constexpr quint8 ALLOC = 10;
} // namespace status

} // namespace amule::ec

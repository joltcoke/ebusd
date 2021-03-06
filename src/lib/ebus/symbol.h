/*
 * Copyright (C) John Baier 2014-2015 <ebusd@johnm.de>
 *
 * This file is part of ebusd.
 *
 * ebusd is free software: you can redistribute it and/or modify
 * it under the terms of the GNU General Public License as published by
 * the Free Software Foundation, either version 3 of the License, or
 * (at your option) any later version.
 *
 * ebusd is distributed in the hope that it will be useful,
 * but WITHOUT ANY WARRANTY; without even the implied warranty of
 * MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE. See the
 * GNU General Public License for more details.
 *
 * You should have received a copy of the GNU General Public License
 * along with ebusd. If not, see http://www.gnu.org/licenses/.
 */

#ifndef LIBEBUS_SYMBOL_H_
#define LIBEBUS_SYMBOL_H_

#include "result.h"
#include <cstring>
#include <cstdlib>
#include <sstream>
#include <queue>

/** \file symbol.h */

using namespace std;

static const unsigned char ESC = 0xA9;       //!< escape symbol, either followed by 0x00 for the value 0xA9, or 0x01 for the value 0xAA
static const unsigned char SYN = 0xAA;       //!< synchronization symbol
static const unsigned char ACK = 0x00;       //!< positive acknowledge
static const unsigned char NAK = 0xFF;       //!< negative acknowledge
static const unsigned char BROADCAST = 0xFE; //!< the broadcast destination address


/**
 * A string of escaped or unescaped bus symbols.
 */
class SymbolString
{

public:
	/**
	 * Creates a new empty escaped or unescaped instance.
	 * @param escaped whether to create an escaped instance.
	 */
	SymbolString(const bool escaped=true) : m_unescapeState(escaped ? 0 : 1), m_crc(0) {}

	/**
	 * Add all symbols from the other @a SymbolString and the calculated CRC if escaped.
	 * @param str the @a SymbolString to copy from.
	 */
	void addAll(const SymbolString& str);

	/**
	 * Parse the escaped or unescaped hex @a string, add all symbols, and add the calculated CRC if escaped.
	 * @param str the hex @a string.
	 * @param isEscaped whether the hex string is escaped.
	 * @return @a RESULT_OK on success, or an error code.
	 */
	result_t parseHex(const string& str, const bool isEscaped=false);

	/**
	 * Returns the symbols as hex string.
	 * @param unescape whether to unescape an escaped instance.
	 * @param skipLastSymbol whether to skip the last symbol (probably the CRC).
	 * @return the symbols as hex string.
	 */
	const string getDataStr(const bool unescape=true, const bool skipLastSymbol=true);

	/**
	 * Returns a reference to the symbol at the specified index.
	 * @param index the index of the symbol to return.
	 * @return the reference to the symbol at the specified index.
	 */
	unsigned char& operator[](const size_t index) { if (index >= m_data.size()) m_data.resize(index+1, 0); return m_data[index]; }

	/**
	 * Returns whether this instance is equal to the other instance.
	 * @param other the other instance.
	 * @return true if this instance is equal to the other instance (i.e. both escaped or both unescaped and same symbols).
	 */
	bool operator==(SymbolString& other) { return m_unescapeState==other.m_unescapeState && m_data==other.m_data; }

	/**
	 * Returns whether this instance is different from the other instance.
	 * @param other the other instance.
	 * @return true if this instance is different from the other instance.
	 */
	bool operator!=(SymbolString& other) { return m_unescapeState!=other.m_unescapeState || m_data!=other.m_data; }

	/**
	 * Appends a the symbol to the end of the symbol string and escapes/unescapes it if necessary.
	 * @param value the symbol to append.
	 * @param isEscaped whether the symbol is escaped.
	 * @param updateCRC whether to update the calculated CRC in @a m_crc.
	 * @return RESULT_OK if another symbol was appended,
	 * RESULT_IN_ESC if this is an unescaped instance and the symbol is escaped and the start of the escape sequence was received,
	 * RESULT_ERR_ESC if this is an unescaped instance and an invalid escaped sequence was detected.
	 */
	result_t push_back(const unsigned char value, const bool isEscaped=true, const bool updateCRC=true);

	/**
	 * Returns the number of symbols in this symbol string.
	 * @return the number of available symbols.
	 */
	unsigned char size() const { return (unsigned char)m_data.size(); }

	/**
	 * Returns the calculated CRC.
	 * @return the calculated CRC.
	 */
	unsigned char getCRC() const { return m_crc; }

	/**
	 * Clear the symbols.
	 */
	void clear() { m_data.clear(); m_unescapeState = m_unescapeState==0 ? 0 : 1; m_crc = 0; }

	/**
	 * Clear the symbols and adjust the escape mode.
	 * @param escape true to set to an escaped instance, false to set to an unescaped instance.
	 */
	void clear(const bool escape) { m_data.clear(); m_unescapeState = escape ? 0 : 1; m_crc = 0; }

private:

	/**
	 * Hidden copy constructor.
	 * @param str the @a SymbolString to copy from.
	 */
	SymbolString(const SymbolString& str)
		: m_data(str.m_data), m_unescapeState(str.m_unescapeState), m_crc(str.m_crc) {}

	/**
	 * Update the calculated CRC in @a m_crc by adding a value.
	 * @param value the (escaped) value to add to the calculated CRC in @a m_crc.
	 */
	void addCRC(const unsigned char value);

	/** the string of bus symbols. */
	vector<unsigned char> m_data;

	/**
	 * 0 if the symbols in @a m_data are escaped,
	 * 1 if the symbols in @a m_data are unescaped and the last symbol passed to @a push_back was a normal symbol,
	 * 2 if the symbols in @a m_data are unescaped and the last symbol passed to @a push_back was the escape symbol.
	 */
	int m_unescapeState;

	/** the calculated CRC. */
	unsigned char m_crc;
};


/**
 * Returns whether the address is one of the 25 master addresses.
 * @param addr the address to check.
 * @return <code>true</code> if the specified address is a master address.
 */
bool isMaster(unsigned char addr);

/**
 * Returns whether the address is a slave address of one of the 25 masters.
 * @param addr the address to check.
 * @return <code>true</code> if the specified address is a slave address of a master.
 */
bool isSlaveMaster(unsigned char addr);

/**
 * Returns the master address associated with the specified address (master or slave).
 * @param addr the address to check.
 * @return the master address, or SYN if the specified address is neither a master address nor a slave address of a master.
 */
unsigned char getMasterAddress(unsigned char addr);

/**
 * Returns the number of the master if the address is a valid bus address.
 * @param addr the bus address.
 * @return the number of the master if the address is a valid bus address (1 to 25), or 0.
 */
unsigned char getMasterNumber(unsigned char addr);

/**
 * Returns whether the address is a valid bus address.
 * @param addr the address to check.
 * @param allowBroadcast whether to also allow @a addr to be the broadcast address (default true).
 * @return <code>true</code> if the specified address is a valid bus address.
 */
bool isValidAddress(unsigned char addr, bool allowBroadcast=true);

#endif // LIBEBUS_SYMBOL_H_

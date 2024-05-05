#pragma once
#include <algorithm>
#include <cstdint>
#include <limits>
#include <iostream>
#include <concepts>
#include <utility>
#include <new>
#include <cstring>
#include <type_traits>

#include "BitSet.hpp"
#include "BitSet.hpp"

// Note: (std::numeric_limits<BlockType>::max)() is used instead of std::numeric_limits<BlockType>::max() because Windows.h defines a macro max which conflicts with std::numeric_limits<BlockType>::max()

namespace woj
{
    /**
	 * Unsigned integer concept
	 * @tparam T Type to check
	 */
    template <typename T>
    concept unsigned_integer = std::is_unsigned_v<T> && std::is_integral_v<T> && !std::is_const_v<T> && !std::is_same_v<bool, T> && /* to suppress warnings */ !std::is_pointer_v<T> && !std::is_reference_v<T>;
    
    /**
     * Character type check
     * @tparam T Type to check
     */
    template <typename T>
    concept char_type = std::is_same_v<T, char> ||
        std::is_same_v<T, wchar_t> ||
        std::is_same_v<T, char16_t> ||
        std::is_same_v<T, char32_t>;

    /**
     * Check if fixed-size container supports read-write iterator
     * @tparam T Type of container
     */
    template <typename T>
    concept fixed_has_read_write_iterator = requires (T a)
    {
        { *a.begin() } -> std::same_as<typename T::reference>;
        { *++a.begin() = typename T::value_type{} };
    };

    /**
     * Check if container supports read iterator
     * @tparam T Type of container
     */
    template <typename T>
    concept has_read_iterator = requires (const T a)
    {
        { *a.cbegin() } -> std::same_as<typename T::const_reference>;
        { *++a.cbegin() } -> std::same_as<typename T::const_reference>;
    };

    /**
     * Check if container supports read iterator
     * @tparam T Type of container
     */
    template <typename T>
    concept dynamic_has_read_write_bracket_operator = requires (T a)
    {
        { T(1)[0] } -> std::same_as<typename T::reference>;
        { T(2)[1] = typename T::value_type{} };
    };

    /**
	 * Check if fixed-size container supports read-write bracket operator
	 * @tparam T Type of container
	 */
    template <typename T>
    concept fixed_has_read_write_bracket_operator = requires (T a)
    {
        { a[0] } -> std::same_as<typename T::reference>;
        { a[a.size() - 1] = typename T::value_type{} };
    };

    /**
     * Check if container supports read bracket operator
     * @tparam T Type of container
     */
    template <typename T>
    concept has_read_bracket_operator = requires (const T a)
    {
        { a[0] } -> std::same_as<typename T::const_reference>;
        { a[a.size() - 1] } -> std::same_as<typename T::const_reference>;
    };

    /**
     * Check if container supports read bracket operator
     * @tparam T Type of container
     */
    template <typename T>
    concept dynamic_has_read_write_iterator = requires (T a)
    {
        { *T(1).begin() } -> std::same_as<typename T::reference>;
        { *++T(2).begin() = typename T::value_type{} };
    };

    namespace bitset_helpers
    {
	    /**
	     * Block type helper class
	     * @tparam BlockType Type of block to use for bit storage
	     */
	    template <unsigned_integer BlockType>
        class block
        {
        public:
            constexpr block() noexcept = delete;
            constexpr block(const block&) noexcept = delete;
	        constexpr block(block&&) noexcept = delete;
	        constexpr ~block() noexcept = delete;

	        /**
	         * Casts block to BlockType (e.g. 0b11111111u == 255 [uint32_t], block<uint8_t>::cast(0b11111111u) == 255 [uint8_t])
	         * @param block Block to cast
	         * @return Block casted to BlockType
	         */
	        [[nodiscard]] static inline constexpr BlockType cast(const BlockType& block)
            {
                return block;
            }

	        /**
	         * Creates a block with all bits filled with the specified value
	         * @param value Value to set the fill the block with
	         * @return New block with all bits filled with the specified value
	         */
	        [[nodiscard]] static inline constexpr BlockType fill(const bool& value)
            {
                return value ? (std::numeric_limits<BlockType>::max)() : 0;
            }

	        /**
	         * Creates a block with all bits set
	         * @return New block with all bits set
	         */
	        [[nodiscard]] static consteval BlockType set()
            {
                return (std::numeric_limits<BlockType>::max)();
            }

	        /**
	         * Creates a block with all bits clear
	         * @return New block with all bits clear
	         */
	        [[nodiscard]] static consteval BlockType clear()
            {
                return BlockType{ 0u };
            }
        };

    }

    /**
     * Fixed-size BitSet class
     * Naming used across documentation:
     * bit value: may be either 1 (true) or 0 (false)
     * bit index: is index of a value 1 (true) or 0 (false) in the bitset, e.g. BlockType=size_t, Size=100, bit index is value between (0-99)
     * bit size/count: size in bits. e.g. 1 uint32_t holds 32 bits = 32 bit size/count
     * block value: Is BlockType value, used to directly copy bits inside it. e.g. when BlockType=uint8_t, block value may be 0b10101010 \n
     * (Visible order of bits is reversed order of the actual bits, e.g. 0b00001111 = 16, so it's lower 4 bits are set)\n
     * block index: index of BlockType value, e.g. when BlockType=size_t, Size=128, block index is value between (0-1), because 128 bits fit into 2 size_t's
     * @tparam BlockType Type of block to use for bit storage (may be one of uint8_t, uint16_t, uint32_t, size_t, {unsigned _int128})
     * @tparam Size Size of bitset, in bits
     */
    template <unsigned_integer BlockType, std::size_t Size>
    class bitset
    {
    public:

        // Type definitions

        // Reference types
        class reference;
        typedef bool const_reference;

        // Iterators
        class iterator;
        class const_iterator;

        // Reverse iterators
        class reverse_iterator;
        class const_reverse_iterator;

        // Value types
        typedef bool value_type;
        typedef bool const_value_type;

        // Other types
        typedef std::size_t size_t;
        typedef size_t difference_type;
        typedef size_t size_type;
        using block_type = BlockType;

        class reference
        {
        public:
            // Reference types


            /**
             * Empty constructor (only here to allow for some concepts to be executed)
             */
            constexpr reference() noexcept : m_bitset(bitset<uint8_t, 8>(0b11111111)), m_index(0) {}

            /**
             * Constructs a new reference instance based on the specified bitset and index
             * @param bitset The bitset instance to iterate over
             * @param index Index of the bit to start the iteration from (bit index)
             */
            constexpr reference(bitset& bitset, const size_t& index) noexcept : m_bitset(bitset), m_index(index) {}

            /**
             * Copy constructor
             */
            constexpr reference(const reference&) noexcept = default;

            /**
             * Destructor
             */
            constexpr ~reference() noexcept = default;

            /**
             * Bool conversion operator
             * @return Value at current index (bit value)
             */
            [[nodiscard]] constexpr operator bool() const noexcept
            {
                return m_bitset.test(m_index);
            }

            /**
             * Assignment operator
             * @param value Value to set the bit to (bit value)
             */
            constexpr reference& operator=(const bool& value) noexcept
            {
                m_bitset.set(m_index, value);
                return *this;
            }

            /**
             * Assignment operator
             * @param other Other reference instance to copy from
             */
        	constexpr reference& operator=(const reference& other) noexcept
            {
                m_bitset.set(m_index, other.m_bitset.test(other.m_index));
                return *this;
            }

            /**
             * Bitwise AND assignment operator
             * @param value Value to AND the bit with (bit value)
             */
            constexpr reference& operator&=(const bool& value) const noexcept
            {
                m_bitset.set(m_index, m_bitset.test(m_index) & value);
                return *this;
            }

            /**
             * Bitwise OR assignment operator
             * @param value Value to OR the bit with (bit value)
             */
            constexpr reference& operator|=(const bool& value) const noexcept
            {
                m_bitset.set(m_index, m_bitset.test(m_index) | value);
                return *this;
            }

            /**
             * Bitwise XOR assignment operator
             * @param value Value to XOR the bit with (bit value)
             */
            constexpr reference& operator^=(const bool& value) const noexcept
            {
                m_bitset.set(m_index, m_bitset.test(m_index) ^ value);
                return *this;
            }

            /**
             * Flip the bit at the current index
             */
            constexpr void flip() const noexcept
            {
                m_bitset.flip(m_index);
            }

            /**
             * Set the bit at the current index to value
             * @param value Value to set the bit to (bit value)
             */
            constexpr void set(const bool& value = true) const noexcept
            {
	            m_bitset.set(m_index, value);
			}

            /**
             * Reset the bit at the current index to value
             */
            constexpr void clear() const noexcept
            {
                m_bitset.clear(m_index);
            }

            bitset& m_bitset;
            const size_t& m_index;
        };

        /**
         * Base iterator class template.
         * @tparam BitSetTypeSpecifier Full type of the bitset to iterate over. [bitset&] or [const bitset&]
         */
        template <typename BitSetTypeSpecifier> requires std::is_same_v<bitset, std::remove_cvref_t<BitSetTypeSpecifier>>
        class iterator_base {
        public:
            constexpr iterator_base() noexcept = delete;

            /**
             * Constructs a new iterator_base instance based on the specified bitset and index.
             * @param bitset The bitset instance to iterate over.
             * @param index Index of the bit to start the iteration from (bit index).
             */
            explicit constexpr iterator_base(BitSetTypeSpecifier bitset, const size_t& index = 0) noexcept : m_bitset(bitset), m_index(index) {}

            /**
             * Copy constructor.
             * @param other Other iterator_base instance to copy from.
             */
            constexpr iterator_base(const iterator_base& other) noexcept : m_bitset(other.m_bitset), m_index(other.m_index) {}

            /**
             * Destructor.
             */
            constexpr ~iterator_base() noexcept = default;

            /**
             * Copy assignment operator.
             */
            constexpr iterator_base& operator=(const iterator_base& other) noexcept {
                m_bitset = other.m_bitset;
                m_index = other.m_index;
                return *this;
            }

            BitSetTypeSpecifier m_bitset;
            size_t m_index;
        };

        /**
         * Iterator class.
         */
        class iterator : public iterator_base<bitset&> {
        public:
            using iterator_base<bitset&>::iterator_base;

            /**
             * Conversion constructor to const_iterator.
             * @param other Other const_iterator instance to convert from.
             */
            constexpr iterator(const const_iterator& other) noexcept : iterator_base<bitset&>(other.m_bitset, other.m_index) {}

            /**
             * Conversion assign operator to const_iterator.
             * @param other Other const_iterator instance to convert from.
             */
            constexpr iterator& operator=(const const_iterator& other) noexcept {
                this->m_bitset = other.m_bitset;
                this->m_index = other.m_index;
                return *this;
            }

            /**
             * Dereference operator (returns the value of the bit at the current bit index).
             * @return Reference to the bit at the current index.
             */
            [[nodiscard]] constexpr reference operator*() noexcept {
                return reference(this->m_bitset, this->m_index);
            }

            /**
             * Increments the iterator to the next bit, pre-increment.
             */
            constexpr iterator& operator++() noexcept
            {
                ++this->m_index;
                return *this;
            }

            /**
			 * Increments the iterator to the next bit, post-increment
			 */
            constexpr iterator& operator++(int) noexcept
            {
                iterator tmp = *this;
                ++this->m_index;
                return tmp;
            }

            /**
             * Decrements the iterator to the previous bit, pre-decrement.
             */
            constexpr iterator& operator--() noexcept
            {
                --this->m_index;
                return *this;
            }

            /**
			 * Decrements the iterator to the previous bit, post-decrement.
			 */
            constexpr iterator& operator--(int) noexcept
            {
                iterator tmp = *this;
                --this->m_index;
                return tmp;
            }

            /**
             * Returns the iterator with the specified amount added to it
             * @param amount Amount to add to the iterator (bit count)
             * @return New iterator instance with the specified amount added to it
             */
            [[nodiscard]] constexpr iterator operator+(const size_t& amount) const noexcept
            {
                return iterator(this->m_bitset, this->m_index + amount);
            }

            /**
             * Returns the iterator with the specified amount subtracted from it
             * @param amount Amount to subtract from the iterator (bit count)
             * @return New iterator instance with the specified amount subtracted from it
             */
            [[nodiscard]] constexpr iterator operator-(const size_t& amount) const noexcept
            {
                return iterator(this->m_bitset, this->m_index - amount);
            }

            /**
             * Returns difference between two iterators
             * @param other Other iterator instance subtract with
             * @return Difference between two iterators
             */
            [[nodiscard]] constexpr difference_type operator-(const iterator& other) const noexcept
            {
                return this->m_index - other.m_index;
            }

            /**
			 * Returns difference between two iterators
			 * @param other Other iterator instance subtract with
			 * @return Difference between two iterators
			 */
            [[nodiscard]] constexpr difference_type operator-(const const_iterator& other) const noexcept
            {
                return this->m_index - other.m_index;
            }

            /**
             * Returns the iterator multiplied by the specified amount
             * @param amount Amount to multiply the iterator by (bit count ?)
             * @return New iterator instance multiplied by the specified amount
             */
            [[nodiscard]] constexpr iterator operator*(const size_t& amount) const noexcept
            {
                return iterator(this->m_bitset, this->m_index * amount);
            }


            /**
             * Returns the iterator divided by the specified amount
             * @param amount Amount to multiply the iterator by (bit count ?)
             * @return New iterator instance divided by the specified amount
             */
            [[nodiscard]] constexpr iterator operator/(const size_t& amount) const noexcept
            {
                return iterator(this->m_bitset, this->m_index / amount);
            }

            /**
             * Adds the specified amount to the iterator
             * @param amount Amount to add to the iterator (bit count)
             */
            constexpr iterator& operator+=(const size_t& amount) noexcept
            {
                this->m_index += amount;
                return *this;
            }

            /**
             * Subtracts the specified amount to the iterator
             * @param amount Amount to add to the iterator (bit count)
             */
            constexpr iterator& operator-=(const size_t& amount) noexcept
            {
                this->m_index -= amount;
                return *this;
            }

            /**
             * Adds the specified amount to the iterator
             * @param amount Amount to add to the iterator (bit count)
             */
            constexpr iterator& operator*=(const size_t& amount) noexcept
            {
                this->m_index *= amount;
                return *this;
            }

            /**
             * Subtracts the specified amount to the iterator
             * @param amount Amount to add to the iterator (bit count)
             */
            constexpr iterator& operator/=(const size_t& amount) noexcept
            {
                this->m_index /= amount;
                return *this;
            }

            /**
             * Inequality operator
             * @param other Other iterator instance to compare with
             * @return true if the iterators are not equal, false otherwise
             */
            [[nodiscard]] constexpr bool operator!=(const iterator& other) const noexcept
            {
                return this->m_index != other.m_index;
            }

            /**
             *  Less than operator
             *  @param other Other iterator instance to compare with
             *  @return true if the current iterator is less than the other iterator, false otherwise
             */
            [[nodiscard]] constexpr bool operator<(const iterator& other) const noexcept
            {
                return this->m_index < other.m_index;
            }

            /**
             *  Less than or equal operator
             *  @param other Other iterator instance to compare with
             *  @return true if the current iterator is less than or equal to the other iterator, false otherwise
             */
            [[nodiscard]] constexpr bool operator<=(const iterator& other) const noexcept
            {
                return this->m_index <= other.m_index;
            }

            /**
             *  Equality operator
             *  @param other Other iterator instance to compare with
             *  @return true if the iterators are equal, false otherwise
             */
            [[nodiscard]] constexpr bool operator==(const iterator& other) const noexcept
            {
                return this->m_index == other.m_index;
            }

            /**
             *  Bigger than or equal operator
             *  @param other Other iterator instance to compare with
             *  @return true if the current iterator is bigger than or equal to the other iterator, false otherwise
             */
            [[nodiscard]] constexpr bool operator>=(const iterator& other) const noexcept
            {
                return this->m_index >= other.m_index;
            }

            /**
             *  Bigger than operator
             *  @param other Other iterator instance to compare with
             *  @return true if the current iterator is bigger than the other iterator, false otherwise
             */
            [[nodiscard]] constexpr bool operator>(const iterator& other) const noexcept
            {
                return this->m_index > other.m_index;
            }
        };

        /**
         * Const iterator class
         */
        class const_iterator : public iterator_base<const bitset&>
        {
        public:
            using iterator_base<const bitset&>::iterator_base;

            /**
             * Dereference operator (returns the value of the bit at the current bit index)
             * @return Reference to the bit at the current index
             */
            [[nodiscard]] constexpr const_reference operator*() const noexcept
            {
                return this->m_bitset.test(this->m_index);
            }

            /**
             * Increments the iterator to the next bit, pre-increment
             */
             constexpr const_iterator& operator++() noexcept
            {
                ++this->m_index;
                return *this;
            }

            /**
             * Increments the iterator to the next bit, post-increment
             */
             constexpr const_iterator& operator++(int) noexcept
            {
                 const_iterator tmp = *this;
                ++this->m_index;
                return tmp;
            }

            /**
             * Decrements the iterator to the previous bit, pre-decrement
             */
             constexpr const_iterator& operator--() noexcept
            {
                --this->m_index;
                return *this;
            }

            /**
             * Decrements the iterator to the previous bit, post-decrement
             */
             constexpr const_iterator& operator--(int) noexcept
            {
                 const_iterator tmp = *this;
                --this->m_index;
                return tmp;
            }

            /**
             * Returns the iterator with the specified amount added to it
             * @param amount Amount to add to the iterator (bit count)
             * @return New iterator instance with the specified amount added to it
             */
            [[nodiscard]]  constexpr const_iterator operator+(const size_t& amount) const noexcept
            {
                return const_iterator(this->m_bitset, this->m_index + amount);
            }

            /**
             * Returns the iterator with the specified amount subtracted from it
             * @param amount Amount to subtract from the iterator (bit count)
             * @return New iterator instance with the specified amount subtracted from it
             */
            [[nodiscard]]  constexpr const_iterator operator-(const size_t& amount) const noexcept
            {
                return const_iterator(this->m_bitset, this->m_index - amount);
            }

            /**
             * Returns difference between two iterators
             * @param other Other iterator instance subtract with
             * @return Difference between two iterators
             */
            [[nodiscard]] constexpr difference_type operator-(const const_iterator& other) const noexcept
            {
                return this->m_index - other.m_index;
            }

            /**
			 * Returns difference between two iterators
			 * @param other Other iterator instance subtract with
			 * @return Difference between two iterators
			 */
            [[nodiscard]] constexpr difference_type operator-(const iterator& other) const noexcept
            {
                return this->m_index - other.m_index;
            }

            /**
             * Returns the iterator multiplied by the specified amount
             * @param amount Amount to multiply the iterator by (bit count ?)
             * @return New iterator instance multiplied by the specified amount
             */
            [[nodiscard]]  constexpr const_iterator operator*(const size_t& amount) const noexcept
            {
                return const_iterator(this->m_bitset, this->m_index * amount);
            }


            /**
             * Returns the iterator divided by the specified amount
             * @param amount Amount to multiply the iterator by (bit count ?)
             * @return New iterator instance divided by the specified amount
             */
            [[nodiscard]]  constexpr const_iterator operator/(const size_t& amount) const noexcept
            {
                return const_iterator(this->m_bitset, this->m_index / amount);
            }

            /**
             * Adds the specified amount to the iterator
             * @param amount Amount to add to the iterator (bit count)
             */
             constexpr const_iterator& operator+=(const size_t& amount) noexcept
            {
                this->m_index += amount;
                return *this;
            }

            /**
             * Subtracts the specified amount to the iterator
             * @param amount Amount to add to the iterator (bit count)
             */
             constexpr const_iterator& operator-=(const size_t& amount) noexcept
            {
                this->m_index -= amount;
                return *this;
            }

            /**
             * Adds the specified amount to the iterator
             * @param amount Amount to add to the iterator (bit count)
             */
             constexpr const_iterator& operator*=(const size_t& amount) noexcept
            {
                this->m_index *= amount;
                return *this;
            }

            /**
             * Subtracts the specified amount to the iterator
             * @param amount Amount to add to the iterator (bit count)
             */
             constexpr const_iterator& operator/=(const size_t& amount) noexcept
            {
                this->m_index /= amount;
                return *this;
            }

            /**
             * Inequality operator
             * @param other Other iterator instance to compare with
             * @return true if the iterators are not equal, false otherwise
             */
            [[nodiscard]] constexpr bool operator!=(const const_iterator& other) const noexcept
            {
                return this->m_index != other.m_index;
            }

            /**
             *  Less than operator
             *  @param other Other iterator instance to compare with
             *  @return true if the current iterator is less than the other iterator, false otherwise
             */
            [[nodiscard]] constexpr bool operator<(const const_iterator& other) const noexcept
            {
                return this->m_index < other.m_index;
            }

            /**
             *  Less than or equal operator
             *  @param other Other iterator instance to compare with
             *  @return true if the current iterator is less than or equal to the other iterator, false otherwise
             */
            [[nodiscard]] constexpr bool operator<=(const const_iterator& other) const noexcept
            {
                return this->m_index <= other.m_index;
            }

            /**
             *  Equality operator
             *  @param other Other iterator instance to compare with
             *  @return true if the iterators are equal, false otherwise
             */
            [[nodiscard]] constexpr bool operator==(const const_iterator& other) const noexcept
            {
                return this->m_index == other.m_index;
            }

            /**
             *  Bigger than or equal operator
             *  @param other Other iterator instance to compare with
             *  @return true if the current iterator is bigger than or equal to the other iterator, false otherwise
             */
            [[nodiscard]] constexpr bool operator>=(const const_iterator& other) const noexcept
            {
                return this->m_index >= other.m_index;
            }

            /**
             *  Bigger than operator
             *  @param other Other iterator instance to compare with
             *  @return true if the current iterator is bigger than the other iterator, false otherwise
             */
            [[nodiscard]] constexpr bool operator>(const const_iterator& other) const noexcept
            {
                return this->m_index > other.m_index;
            }
        };

        /**
         * Reverse iterator class
         */
        class reverse_iterator : public iterator_base<bitset&>
        {
        public:
            using iterator_base<bitset&>::iterator_base;

            /**
             * Conversion constructor to const_reverse_iterator
             * @param other Other const_reverse_iterator instance to convert from
             */
            constexpr reverse_iterator(const const_reverse_iterator& other) noexcept : iterator_base<bitset&>(other.m_bitset, other.m_index) {}

            /**
             * Conversion assign operator to const_reverse_iterator
             * @param other Other const_reverse_iterator instance to convert from
             */
            constexpr reverse_iterator& operator=(const const_reverse_iterator& other) noexcept
            {
                this->m_bitset = other.m_bitset;
                this->m_index = other.m_index;
                return *this;
            }

            /**
             * Dereference operator (returns the value of the bit at the current bit index)
             * @return Reference to the bit at the current index
             */
            [[nodiscard]] reference operator*() noexcept
            {
                return reference(this->m_bitset, this->m_index);
            }


            /**
             * Increments the reverse_iterator to the next bit
             */
            constexpr reverse_iterator& operator++() noexcept
            {
                --this->m_index;
                return *this;
            }

            /**
			 * Increments the reverse_iterator to the next bit, post-increment
			 */
            constexpr reverse_iterator& operator++(int) noexcept
            {
                reverse_iterator tmp = *this;
                --this->m_index;
                return tmp;
            }

            /**
             * Decrements the reverse_iterator to the previous bit
             */
            constexpr reverse_iterator& operator--() noexcept
            {
                ++this->m_index;
                return *this;
            }



            /**
             * Decrements the reverse_iterator to the previous bit, post-decrement
             */
            constexpr reverse_iterator& operator--(int) noexcept
            {
                reverse_iterator tmp = *this;
                ++this->m_index;
                return tmp;
            }

            /**
             * Returns the reverse_iterator with the specified amount added to it
             * @param amount Amount to add to the reverse_iterator (bit count)
             * @return New reverse_iterator instance with the specified amount added to it
             */
            [[nodiscard]] constexpr reverse_iterator&& operator+(const size_t& amount) const noexcept
            {
                return reverse_iterator(this->m_bitset, this->m_index - amount);
            }
            /**
             * Returns the reverse_iterator with the specified amount subtracted from it
             * @param amount Amount to subtract from the reverse_iterator (bit count)
             * @return New reverse_iterator instance with the specified amount subtracted from it
             */
            [[nodiscard]] constexpr reverse_iterator&& operator-(const size_t& amount) const noexcept
            {
                return reverse_iterator(this->m_bitset, this->m_index + amount);
            }

            /**
			 * Returns difference between two iterators
			 * @param other Other iterator instance subtract with
			 * @return Difference between two iterators
			 */
            [[nodiscard]] constexpr difference_type operator-(const reverse_iterator& other) const noexcept
            {
                return this->m_index + other.m_index;
            }

            /**
             * Returns difference between two iterators
             * @param other Other iterator instance subtract with
             * @return Difference between two iterators
             */
            [[nodiscard]] constexpr difference_type operator-(const const_reverse_iterator& other) const noexcept
            {
                return this->m_index + other.m_index;
            }

            /**
             * Adds the specified amount to the reverse_iterator
             * @param amount Amount to add to the reverse_iterator (bit count)
             */
            constexpr reverse_iterator& operator+=(const size_t& amount) const noexcept
            {
                this->m_index -= amount;
                return *this;
            }

            /**
             * Subtracts the specified amount to the reverse_iterator
             * @param amount Amount to add to the reverse_iterator (bit count)
             */
            constexpr reverse_iterator& operator-=(const size_t& amount) const noexcept
            {
                this->m_index += amount;
                return *this;
            }

            /**
             * Inequality operator
             * @param other Other reverse_iterator instance to compare against
             * @return True if the instances are not equal, false otherwise
             */
            [[nodiscard]] constexpr bool operator!=(const reverse_iterator& other) const noexcept
            {
                return this->m_index != other.m_index;
            }

            /**
             *  Less than operator
             *  @param other Other reverse_iterator instance to compare against
             *  @return True if the current instance is less than the other instance, false otherwise
             */
            [[nodiscard]] constexpr bool operator<(const reverse_iterator& other) const noexcept
            {
                return this->m_index > other.m_index;
            }

            /**
             *  Less than or equal operator
             *  @param other Other reverse_iterator instance to compare against
             *  @return True if the current instance is less than or equal to the other instance, false otherwise
             */
            [[nodiscard]] constexpr bool operator<=(const reverse_iterator& other) const noexcept
            {
                return this->m_index >= other.m_index;
            }

            /**
             *  Equality operator
             *  @param other Other reverse_iterator instance to compare against
             *  @return True if the instances are equal, false otherwise
             */
            [[nodiscard]] constexpr bool operator==(const reverse_iterator& other) const noexcept
            {
                return this->m_index == other.m_index;
            }

            /**
             *  Bigger than or equal operator
             *  @param other Other reverse_iterator instance to compare against
             *  @return True if the current instance is bigger than or equal to the other instance, false otherwise
             */
            [[nodiscard]] constexpr bool operator>=(const reverse_iterator& other) const noexcept
            {
                return this->m_index >= other.m_index;
            }

            /**
             *  Bigger than operator
             *  @param other Other reverse_iterator instance to compare against
             *  @return True if the current instance is bigger than the other instance, false otherwise
             */
            [[nodiscard]] constexpr bool operator>(const reverse_iterator& other) const noexcept
            {
                return this->m_index < other.m_index;
            }
        };

        /**
         * Reverse iterator class
         */
        class const_reverse_iterator : public iterator_base<const bitset&>
        {
        public:
            using iterator_base<const bitset&>::iterator_base;

            /**
             * Dereference operator (returns the value of the bit at the current bit index)
             * @return Value of the bit at the current index (bit value)
             */
            [[nodiscard]] const_reference operator*() const noexcept
            {
                return this->m_bitset.test(this->m_index);
            }


            /**
             * Increments the reverse_iterator to the next bit
             */
            constexpr const_reverse_iterator& operator++() noexcept
            {
                --this->m_index;
                return *this;
            }

            /**
			 * Increments the reverse_iterator to the next bit, post-increment
			 */
            constexpr const_reverse_iterator& operator++(int) noexcept
            {
                const_reverse_iterator tmp = *this;
                --this->m_index;
                return tmp;
            }

            /**
             * Decrements the reverse_iterator to the previous bit
             */
            constexpr const_reverse_iterator& operator--() noexcept
            {
                ++this->m_index;
                return *this;
            }

            /**
             * Decrements the reverse_iterator to the previous bit, post-decrement
             */
            constexpr const_reverse_iterator& operator--(int) noexcept
            {
                const_reverse_iterator tmp = *this;
                ++this->m_index;
                return tmp;
            }

            /**
             * Returns the reverse_iterator with the specified amount added to it
             * @param amount Amount to add to the reverse_iterator (bit count)
             * @return New reverse_iterator instance with the specified amount added to it
             */
            [[nodiscard]] constexpr const_reverse_iterator&& operator+(const size_t& amount) const noexcept
            {
                return const_reverse_iterator(this->m_index, this->m_index - amount);
            }
            /**
             * Returns the reverse_iterator with the specified amount subtracted from it
             * @param amount Amount to subtract from the reverse_iterator (bit count)
             * @return New reverse_iterator instance with the specified amount subtracted from it
             */
            [[nodiscard]] constexpr const_reverse_iterator&& operator-(const size_t& amount) const noexcept
            {
                return const_reverse_iterator(this->m_index, this->m_index + amount);
            }

            /**
			 * Returns difference between two iterators
			 * @param other Other iterator instance subtract with
			 * @return Difference between two iterators
			 */
            [[nodiscard]] constexpr difference_type operator-(const const_reverse_iterator& other) const noexcept
            {
                return this->m_index - other.m_index;
            }

            /**
             * Returns difference between two iterators
             * @param other Other iterator instance subtract with
             * @return Difference between two iterators
             */
            [[nodiscard]] constexpr difference_type operator-(const reverse_iterator& other) const noexcept
            {
                return this->m_index - other.m_index;
            }

            /**
             * Adds the specified amount to the reverse_iterator
             * @param amount Amount to add to the reverse_iterator (bit count)
             */
            constexpr const_reverse_iterator& operator+=(const size_t& amount) noexcept
            {
                this->m_index -= amount;
                return *this;
            }

            /**
             * Subtracts the specified amount to the reverse_iterator
             * @param amount Amount to add to the reverse_iterator (bit count)
             */
            constexpr const_reverse_iterator& operator-=(const size_t& amount) noexcept
            {
                this->m_index += amount;
                return *this;
            }

            /**
             * Inequality operator
             * @param other Other const_reverse_iterator instance to compare against
             * @return true if instances are not equal, false otherwise
             */
            [[nodiscard]] constexpr bool operator!=(const const_reverse_iterator& other) const noexcept
            {
                return this->m_index != other.m_index;
            }

            /**
             *  Less than operator
             *  @param other Other const_reverse_iterator instance to compare against
             *  @return true if the current instance is less than the other instance, false otherwise
             */
            [[nodiscard]] constexpr bool operator<(const const_reverse_iterator& other) const noexcept
            {
                return this->m_index > other.m_index;
            }

            /**
             *  Less than or equal operator
             *  @param other Other const_reverse_iterator instance to compare against
             *  @return true if the current instance is less than or equal to the other instance, false otherwise
             */
            [[nodiscard]] constexpr bool operator<=(const const_reverse_iterator& other) const noexcept
            {
                return this->m_index >= other.m_index;
            }

            /**
             *  Equality operator
             *  @param other Other const_reverse_iterator instance to compare against
             *  @return true if instances are equal, false otherwise
             */
            [[nodiscard]] constexpr bool operator==(const const_reverse_iterator& other) const noexcept
            {
                return this->m_index == other.m_index;
            }

            /**
             *  Bigger than or equal operator
             *  @param other Other const_reverse_iterator instance to compare against
             *  @return true if the current instance is bigger than or equal to the other instance, false otherwise
             */
            [[nodiscard]] constexpr bool operator>=(const const_reverse_iterator& other) const noexcept
            {
                return this->m_index >= other.m_index;
            }

            /**
             *  Bigger than operator
             *  @param other Other const_reverse_iterator instance to compare against
             *  @return true if the current instance is bigger than the other instance, false otherwise
             */
            [[nodiscard]] constexpr bool operator>(const const_reverse_iterator& other) const noexcept
            {
                return this->m_index < other.m_index;
            }
        };

        /**
         * Empty constructor
         */
        constexpr bitset() noexcept : m_data{ 0 } {}

        /**
         * Bool value constructor
         * @tparam U Used to constrain type to bool. [don't use]
         * @param val Bool value to fill the bitset with (bit value)
         */
        template <typename U = BlockType> requires (!std::is_same_v<bool, std::remove_cvref_t<U>> && !std::is_pointer_v<std::remove_cvref_t<U>> && !std::is_array_v<std::remove_cvref_t<U>>)
        explicit constexpr bitset(const bool& val)
        {
            fill(val);
        }

        /**
         * Block value constructor
         * @param block Block to fill the bitset with (block value)
         */
        explicit constexpr bitset(const BlockType& block) noexcept
        {
            fill_block(block);
        }

        /**
		 * Copy constructor
		 * @param other Other bitset instance to copy from
		 */
        constexpr bitset(const bitset& other) noexcept
        {
            from_other(other);
        }

        /**
         * Copy constructor with different size
         * @tparam OtherSize Size of the other bitset to copy from
         * @param other Other bitset instance to copy from
         */
        template <size_t OtherSize> requires (Size != OtherSize)
        constexpr bitset(const bitset<BlockType, OtherSize>& other) noexcept
        {
            from_other<OtherSize>(other);
        }

        /**
         * Conversion constructor with different block type and size
         * @tparam OtherBlockType Type of the block to convert from
         * @tparam OtherSize Size of the other bitset to convert from
         * @param other Other bitset instance to convert from
         */
        template <unsigned_integer OtherBlockType, size_t OtherSize> requires (std::convertible_to<OtherBlockType, BlockType> && !std::is_same_v<BlockType, OtherBlockType>)
        constexpr bitset(const bitset<OtherBlockType, OtherSize>& other) noexcept : m_data{ 0 }
        {
            _from_other<OtherBlockType, OtherSize>(other);
        }

        /**
		 * C string stack array to bitset conversion constructor
		 * @tparam Elem Type of the character array
		 * @tparam StrSize Size of the character array
		 * @param c_str C string stack array to convert from (must be null-terminated)
		 * @param set_chr Character that represents set bits
		 */
        template <char_type Elem, size_t StrSize>
        constexpr bitset(const Elem(&c_str)[StrSize], const Elem& set_chr = '1') noexcept : m_data{ 0 }
        {
            _from_c_string<Elem, StrSize>(c_str, set_chr);
        }

        /**
         * C string pointer array to bitset conversion constructor
         * @tparam PtrArr Type of the character array [don't use]
         * @tparam Elem Type of the character array [don't use]
         * @param c_str Pointer to c string to convert from (must be null-terminated)
         * @param set_chr Character that represents set bits
		 */
        template <typename PtrArr = const char*, char_type Elem = std::remove_cvref_t<std::remove_pointer_t<std::remove_cvref_t<PtrArr>>>> requires (std::is_pointer_v<PtrArr> && !std::is_array_v<PtrArr>)
        bitset(PtrArr c_str, const Elem& set_chr = '1') noexcept : m_data{ 0 }
        {
            _from_c_string<PtrArr, Elem>(c_str, set_chr);
        }

        /**
		 * String to bitset conversion constructor
		 * @tparam Elem Type of the character array
		 * @param str String to convert from (must be null-terminated)
		 * @param set_chr Character that represents set bits
		 */
        template <char_type Elem>
        constexpr bitset(const std::basic_string<Elem>& str, const Elem& set_chr = '1') noexcept : m_data{ 0 }
        {
            _from_string<Elem>(str, set_chr);
        }

        /**
		 * Bool array to bitset conversion constructor
		 * @tparam ArrType Used to constrain type. [don't use]
		 * @tparam OtherSize Size of the bool array to copy from
		 * @param arr Bool array containing bit values to convert from
		 */
        template <typename ArrType = bool, uint64_t OtherSize> requires (std::is_same_v<ArrType, bool>)
        constexpr bitset(const ArrType(&arr)[OtherSize]) noexcept : m_data{ 0 }
        {
            _from_bool_array<OtherSize>(arr);
        }

        /**
         * Bool pointer array to bitset conversion constructor
         * @tparam PtrArr Used to constrain type to const bool* [don't use]
         * @param ptr Pointer to the bool array containing bit values to convert from. It's size must be >= size()
         */
        template <typename PtrArr = const bool*> requires (std::is_pointer_v<PtrArr> && !std::is_array_v<PtrArr>&& std::is_same_v<const bool*, PtrArr> && !std::is_same_v<std::remove_cvref_t<BlockType>, bool*>)
    	bitset(PtrArr ptr) noexcept : m_data{ 0 }
        {
            _from_bool_array<PtrArr>(ptr);
        }

        /**
         * Block stack array copy constructor
         * @tparam OtherSize Size of the other bitset to copy from
         * @param arr Array of block values to initialize the bitset with.
         */
        template <size_t OtherSize> requires (!std::is_same_v<bool, BlockType>)
        constexpr bitset(const BlockType (&arr)[OtherSize]) noexcept
        {
            from_block_array<OtherSize>(arr);
        }

        /**
		 * Block pointer array copy constructor
		 * @tparam PtrArr Type of the pointer array [don't use]
		 * @param ptr Pointer to array of BlockType values to initialize the bitset with (block values). It's size must be >= storage_size()
		 */
        template <typename PtrArr = const BlockType*> requires (std::is_pointer_v<PtrArr> && !std::is_array_v<PtrArr>&& std::is_same_v<BlockType, std::remove_cvref_t<std::remove_pointer_t<std::remove_cvref_t<PtrArr>>>> && !std::is_same_v<bool, std::remove_cvref_t<std::remove_pointer_t<std::remove_cvref_t<PtrArr>>>>)
        bitset(PtrArr ptr) noexcept
        {
            from_block_array<PtrArr>(ptr);
        }

        /**
		 * Block stack array conversion constructor with different block type and size
		 * @tparam OtherBlockType Type of the block to convert from
		 * @tparam OtherSize Size of the other bitset to convert from
		 * @param arr Array of OtherBlockType values to convert from.
		 */
        template <unsigned_integer OtherBlockType, size_t OtherSize> requires (std::convertible_to<OtherBlockType, BlockType> && !std::is_same_v<BlockType, OtherBlockType>)
    	constexpr bitset(const OtherBlockType (&arr)[OtherSize]) noexcept : m_data{ 0 }
        {
            _from_block_array<OtherBlockType, OtherSize>(arr);
        }

        /**
		 * Block stack pointer array conversion constructor with different block type and size
		 * @tparam PtrArr Type of the block pointer array [don't use]
		 * @tparam Elem Type of elem in the block pointer array [don't use]
		 * @param ptr Pointer to array of block values to copy from. It's size must be >= storage_size()
		 */
        template <typename PtrArr, unsigned_integer Elem = std::remove_cvref_t<std::remove_pointer_t<std::remove_cvref_t<PtrArr>>>> requires (std::convertible_to<Elem, BlockType>&& std::convertible_to<BlockType, Elem> && !std::is_same_v<BlockType, Elem>&& std::is_pointer_v<PtrArr> && !std::is_array_v<PtrArr> && !std::is_same_v<bool, Elem>)
        bitset(PtrArr const& ptr) noexcept : m_data{ 0 }
        {
            _from_block_array<PtrArr, Elem>(ptr);
        }

        /**
         * Block container copy constructor
         * @tparam T Type of the container to copy from [must have fixed size equal to Size, value_type equal to BlockType, support const_iterator, support .size()]
         * @param cont Container containing block values to copy from
         */
        template <has_read_iterator T> requires (std::is_same_v<BlockType, typename T::value_type>)
        constexpr bitset(const T& cont) noexcept
        {
            from_block_container<T>(cont);
        }


        /**
         * Destructor
         */
        ~bitset() noexcept = default;

        /**
         * Returns the value of the bit at the specified index
         * @param index Index of the bit to retrieve (bit index)
         * @return Value of the bit at the specified index (bit value)
         */
        [[nodiscard]] const_reference operator[](const size_t& index) const noexcept
        {
            return m_data[index / m_block_size] & BlockType{1} << index % m_block_size;
        }

        /**
         * Returns reference to the value of the bit at the specified index
         * @param index Index of the bit to retrieve (bit index)
         * @return Reference to the value of the bit at the specified index
         */
        [[nodiscard]] reference operator[](const size_t& index) noexcept
        {
            return reference(*this, index);
        }

        // assignment operator

        /**
         * Copy assignment operator
         * @param other Other bitset instance to copy from
         */
        constexpr bitset& operator=(const bitset& other) noexcept
        {
            from_other(other);
            return *this;
        }

        /**
         * Copy assignment operator with different size
         * @tparam OtherSize Size of the other bitset to copy from
         * @param other Other bitset instance to copy from
         */
        template <size_t OtherSize> requires (Size != OtherSize)
        constexpr bitset& operator=(const bitset<BlockType, OtherSize>& other) noexcept
        {
            from_other<OtherSize>(other);
            return *this;
        }

        /**
         * Conversion assignment operator with different block type and size
         * @tparam OtherBlockType Type of the block to convert from
         * @tparam OtherSize Size of the other bitset to convert from
         * @param other Other bitset instance to convert from
         */
        template <unsigned_integer OtherBlockType, size_t OtherSize> requires (std::convertible_to<OtherBlockType, BlockType> && !std::is_same_v<BlockType, OtherBlockType>)
        constexpr bitset& operator=(const bitset<OtherBlockType, OtherSize>& other) noexcept
        {
            clear();
            _from_other<OtherBlockType, OtherSize>(other);
            return *this;
        }

        /**
         * C string stack array to bitset conversion assignment operator
         * @tparam Elem Type of the character array
         * @tparam StrSize Size of the character array
         * @param c_str C string stack array to convert from (must be null-terminated)
         */
        template <char_type Elem, size_t StrSize>
        constexpr bitset& operator=(const Elem(&c_str)[StrSize]) noexcept
        {
            clear();
            _from_c_string<Elem, StrSize>(c_str, '1');
            return *this;
        }

        /**
         * C string pointer array to bitset conversion assignment operator
         * @tparam PtrArr Type of the character array [don't use]
         * @tparam Elem Type of the character array [don't use]
         * @param c_str Pointer to c string to convert from (must be null-terminated)
         */
        template <typename PtrArr = const char*, char_type Elem = std::remove_cvref_t<std::remove_pointer_t<std::remove_cvref_t<PtrArr>>>> requires (std::is_pointer_v<PtrArr> && !std::is_array_v<PtrArr>)
        bitset& operator=(PtrArr c_str) noexcept
        {
            clear();
            _from_c_string<PtrArr, Elem>(c_str, '1');
            return *this;
        }

        /**
         * String to bitset conversion assignment operator
         * @tparam Elem Type of the character array
         * @param str String to convert from (must be null-terminated)
         */
        template <char_type Elem>
        constexpr bitset& operator=(const std::basic_string<Elem>& str) noexcept
        {
            clear();
            _from_string<Elem>(str, '1');
            return *this;
        }

        /**
         * Bool array to bitset conversion assignment operator
         * @tparam OtherSize Size of the bool array to copy from
         * @tparam ArrType Must always be bool. To constrain types. [don't use]
         * @param arr Bool array containing the values to convert from (bit values)
         */
        template <typename ArrType = bool, uint64_t OtherSize> requires (std::is_same_v<bool, ArrType>)
        constexpr bitset& operator=(const ArrType(&arr)[OtherSize]) noexcept
        {
            clear();
            _from_bool_array<OtherSize>(arr);
            return *this;
        }

        /**
         * Bool pointer array to bitset conversion assignment operator
         * @tparam PtrArr Used to constrain type to const bool* [don't use]
         * @param ptr Pointer to the bool array containing the values to convert from (bit values). It's size must be >= size()
         */
        template <typename PtrArr = const bool*> requires (std::is_pointer_v<PtrArr> && !std::is_array_v<PtrArr> && std::is_same_v<const bool*, PtrArr> && !std::is_same_v<std::remove_cvref_t<BlockType>, bool*>)
        bitset& operator=(PtrArr ptr) noexcept
        {
            clear();
            _from_bool_array<PtrArr>(ptr);
            return *this;
        }

        /**
         * Block stack array copy assignment operator
         * @tparam OtherSize Size of the other bitset to copy from
         * @param arr Array of BlockType values to copy from (block values).
         */
        template <size_t OtherSize>
        constexpr bitset& operator=(const BlockType(&arr)[OtherSize]) noexcept
        {
            from_block_array<OtherSize>(arr);
            return *this;
        }

        /**
         * Block pointer array copy assignment operator
         * @tparam PtrArr Type of the pointer array [don't use]
         * @param ptr Pointer to array of BlockType values to initialize the bitset with (block values). It's size must be >= storage_size()
         */
        template <typename PtrArr = const BlockType*> requires std::is_pointer_v<PtrArr> && (!std::is_array_v<PtrArr>&& std::is_same_v<BlockType, std::remove_cvref_t<std::remove_pointer_t<std::remove_cvref_t<PtrArr>>>> && !std::is_same_v<bool, std::remove_cvref_t<std::remove_pointer_t<std::remove_cvref_t<PtrArr>>>>)
        bitset& operator=(PtrArr ptr) noexcept
        {
            from_block_array<PtrArr>(ptr);
            return *this;
        }

        /**
         * Block stack array conversion assignment operator with different block type and size
         * @tparam OtherBlockType Type of the block to convert from
         * @tparam OtherSize Size of the other bitset to convert from
         * @param arr Array of OtherBlockType values to convert from.
         */
        template <unsigned_integer OtherBlockType, size_t OtherSize> requires (std::convertible_to<OtherBlockType, BlockType> && !std::is_same_v<BlockType, OtherBlockType>)
        constexpr bitset& operator=(const OtherBlockType(&arr)[OtherSize]) noexcept
        {
            clear();
            _from_block_array<OtherBlockType, OtherSize>(arr);
            return *this;
        }

        /**
         * Block stack pointer array conversion assignment operator with different block type and size
         * @tparam PtrArr Type of the block pointer array [don't use]
         * @tparam Elem Type of elem in the block pointer array [don't use]
         * @param ptr Pointer to array of block values to convert from. It's size must be >= storage_size()
         */
        template <typename PtrArr, unsigned_integer Elem = std::remove_cvref_t<std::remove_pointer_t<std::remove_cvref_t<PtrArr>>>> requires (std::convertible_to<Elem, BlockType>&& std::convertible_to<BlockType, Elem> && !std::is_same_v<BlockType, Elem>&& std::is_pointer_v<PtrArr> && !std::is_array_v<PtrArr> && !std::is_same_v<bool, Elem>)
        bitset& operator=(PtrArr const& ptr) noexcept
        {
            clear();
            _from_block_array<PtrArr, Elem>(ptr);
            return *this;
        }

        // TODO: General comp & bitwise operators

        // comparison operators

		/**
		 * Equality operator with different size
		 * @param other Other bitset instance to compare with
		 * @return True if the two instances are equal, false otherwise
		 */
        template <typename OtherBlockType, size_t OtherSize> requires (Size != OtherSize)
		[[nodiscard]] consteval bool operator==(const bitset<OtherBlockType, OtherSize>& other) const noexcept
        {
	        return false;
		}
        
        /**
         * Equality operator
         * @param other Other bitset instance to compare with
         * @return True if the two instances are equal, false otherwise
         */
        [[nodiscard]] constexpr bool operator==(const bitset& other) const noexcept
        {
            for (size_t i = 0; i < m_full_storage_size; ++i)
            {
                if (m_data[i] != other.m_data[i])
                    return false;
            }
            if (m_partial_size)
            {
                const BlockType mask = (1 << m_partial_size) - 1;
                if ((m_data[m_storage_size - 1] & mask) != (other.m_data[m_storage_size - 1] & mask))
                    return false;
            }
            return true;
        }

        /**
		 * Inequality operator with different size
		 * @tparam OtherSize Size of the other bitset to compare with
		 * @param other Other bitset instance to compare with
		 * @return True if the two instances are not equal, false otherwise
		 */
        template <typename OtherBlockType, size_t OtherSize> requires (Size != OtherSize)
        [[nodiscard]] consteval bool operator!=(const bitset<OtherBlockType, OtherSize>& other) const noexcept
        {
            return true;
        }

        /**
         * Inequality operator
         * @param other Other bitset instance to compare with
         * @return True if the two instances are not equal, false otherwise
         */
        [[nodiscard]] constexpr bool operator!=(const bitset& other) const noexcept
        {
            for (size_t i = 0; i < m_full_storage_size; ++i)
            {
                if (m_data[i] == other.m_data[i])
                    return false;
            }
            if (m_partial_size)
            {
                const BlockType mask = (1 << m_partial_size) - 1;
                if ((m_data[m_storage_size - 1] & mask) == (other.m_data[m_storage_size - 1] & mask))
                    return false;
            }
            return true;
		}

        // Bitwise operators

        /**
         * Bitwise AND operator
         * @param other Other bitset instance to perform the operation with
         * @return New bitset instance containing the result of the operation
         */
        [[nodiscard]] constexpr bitset operator&(const bitset& other) const noexcept
        {
            bitset result;
            for (size_t i = 0; i < result.m_storage_size; ++i)
                result.m_data[i] = m_data[i] & other.m_data[i];
            return result;
        }

        /**
         * Apply bitwise AND operation with another bitset instance
         * @param other Other bitset instance to perform the operation with
         */
    	constexpr bitset& operator&=(const bitset& other) noexcept
        {
            for (size_t i = 0; i < m_storage_size; ++i)
                m_data[i] &= other.m_data[i];
            return *this;
        }

        /**
         * Bitwise OR operator
         * @param other Other bitset instance to perform the operation with
         * @return New bitset instance containing the result of the operation
         */
        [[nodiscard]] constexpr bitset operator|(const bitset& other) const noexcept
        {
            bitset result;
            for (size_t i = 0; i < result.m_storage_size; ++i)
                result.m_data[i] = m_data[i] | other.m_data[i];
            return result;
        }

        /**
         * Apply bitwise OR operation with another bitset instance
         * @param other Other bitset instance to perform the operation with
         */
        constexpr bitset& operator|=(const bitset& other) noexcept
        {
            for (size_t i = 0; i < m_storage_size; ++i)
                *m_data[i] |= other.m_data[i];
            return *this;
        }

        /**
         * Bitwise XOR operator
         * @param other Other bitset instance to perform the operation with
         * @return New bitset instance containing the result of the operation
         */
        [[nodiscard]] constexpr bitset operator^(const bitset& other) const noexcept
        {
            bitset result;
            for (size_t i = 0; i < result.m_storage_size; ++i)
                result.m_data[i] = m_data[i] ^ other.m_data[i];
            return result;
        }

        /**
         * Apply bitwise XOR operation with another bitset instance
         * @param other Other bitset instance to perform the operation with
         */
        constexpr bitset& operator^=(const bitset& other) noexcept
        {
            for (size_t i = 0; i < m_storage_size; ++i)
                m_data[i] ^= other.m_data[i];
            return *this;
        }

        /**
         * Bitwise NOT operator
         * @return New bitset instance containing the result of the operation
         */
        [[nodiscard]] constexpr bitset operator~() const noexcept
        {
            bitset result;
            for (size_t i = 0; i < m_storage_size; ++i)
                result.m_data[i] = ~m_data[i];
            return result;
        }

        /**
         * Bitwise right shift operator
         * @param shift Amount of bits to shift to the right
         * @return New bitset instance containing the result of the operation
         */
        [[nodiscard]] constexpr bitset operator>>(const size_t& shift) const noexcept
        {
            bitset result;
            if (shift <= m_block_size)
            {
                for (size_t i = 0; i < m_storage_size; ++i)
                    result.m_data[i] = m_data[i] >> shift;
            }

            return result;
        }

        /**
         * Apply bitwise right shift operation
         * @param shift Amount of bits to shift to the right
         */
        constexpr bitset& operator>>=(const size_t& shift) noexcept
        {
            if (shift > m_block_size)
                clear();
            else
            {
                for (size_t i = 0; i < m_storage_size; ++i)
                    m_data[i] >>= shift;
            }
            return *this;
        }


        /**
         * Bitwise right shift operator
         * @param shift Amount of bits to shift to the right
         * @return New bitset instance containing the result of the operation
         */
        [[nodiscard]] constexpr bitset operator<<(const size_t& shift) const noexcept
        {
            bitset result;
            if (shift <= m_block_size)
            {
                for (size_t i = 0; i < m_storage_size; ++i)
                    result.m_data[i] = m_data[i] << shift;
            }
            return result;
        }

        /**
         * Apply bitwise right shift operation
         * @param shift Amount of bits to shift to the right
         */
        constexpr bitset& operator<<=(const size_t& shift) noexcept
        {
            if (shift > m_block_size)
                clear();
            else
            {
                for (size_t i = 0; i < m_storage_size; ++i)
                    m_data[i] <<= shift;
            }
            return *this;
        }

        /**
         * Difference operator
         * @param other Other bitset instance to compare with
         * @return New bitset instance containing the result of the operation
         */
        [[nodiscard]] constexpr bitset operator-(const bitset& other) const noexcept
        {
            bitset result;
            for (size_t i = 0; i < result.m_storage_size; ++i)
                result.m_data[i] = m_data[i] & ~other.m_data[i];
            return result;
        }

        /**
         * Apply difference operation
         * @param other Other bitset instance to compare with
         */
        constexpr bitset& operator-=(const bitset& other) noexcept
        {
            for (size_t i = 0; i < m_storage_size; ++i)
                m_data[i] &= ~other.m_data[i];
            return *this;
        }

        // Conversion functions/operators

        /**
         * Copy function
         * @param other Other bitset instance to copy from
         */
        constexpr void from_other(const bitset& other) noexcept
        {
            if (this != &other)
				std::copy(other.m_data, other.m_data + other.m_storage_size, m_data);
        }

        /**
		 * Copy function with different size
		 * @tparam OtherSize Size of the other bitset to copy from
		 * @param other Other bitset instance to copy from
		 */
        template <size_t OtherSize> requires (Size != OtherSize)
        constexpr void from_other(const bitset<BlockType, OtherSize>& other) noexcept
        {
            size_t min_storage_size = (std::min)(m_storage_size, other.m_storage_size);
            std::copy(other.m_data, other.m_data + min_storage_size, m_data);
            for (size_t i = min_storage_size; i < m_storage_size; ++i)
                m_data[i] = 0;
        }

    private:

        /**
		 * Conversion function with different block type and size
		 * @tparam OtherBlockType Type of the block to convert from
		 * @tparam OtherSize Size of the other bitset to convert from
		 * @param other Other bitset instance to convert from
		 */
        template <unsigned_integer OtherBlockType, size_t OtherSize> requires (std::convertible_to<OtherBlockType, BlockType> && !std::is_same_v<BlockType, OtherBlockType>)
        constexpr void _from_other(const bitset<OtherBlockType, OtherSize>& other) noexcept
        {
            if (sizeof(BlockType) > sizeof(OtherBlockType))
            {
                constexpr uint16_t diff = sizeof(BlockType) / sizeof(OtherBlockType);

                for (size_t i = 0; i < m_storage_size; ++i)
                {
                    for (size_t j = 0; j < diff; ++j)
                    {
                        if (i * diff + j >= other.m_storage_size)
                        {
                            return;
                        }
                        m_data[i] |= static_cast<BlockType>(other.m_data[i * diff + j]) << j * other.m_block_size;
                    }
                }
            }
            else
            {
                constexpr uint16_t diff = sizeof(OtherBlockType) / sizeof(BlockType);

                for (size_t i = 0; i < other.m_storage_size; ++i)
                {
                    for (uint16_t j = 0; j < diff; ++j)
                    {
                        if (i * diff + j >= m_storage_size)
                        {
                            return;
                        }
                        m_data[i * diff + j] = other.m_data[i] >> j % diff * m_block_size;
                    }
                }
            }
        }

        /**
		 * String to bitset conversion function
		 * @tparam Elem Type of the character array
		 * @param str String to convert from (must be null-terminated)
		 * @param set_chr Character that represents set bits
		 */
        template <char_type Elem>
        constexpr void _from_string(const std::basic_string<Elem>& str, const Elem& set_chr = '1') noexcept
        {
            for (size_t i = 0; i < m_storage_size; ++i)
            {
                for (uint16_t j = 0; j < m_block_size; ++j)
                {
                    if (i * m_block_size + j >= str.size())
                    {
                        return;
                    }
                    m_data[i] |= str[i * m_block_size + j] == set_chr ? BlockType{ 1 } << j : 0;
                }
            }
        }

    public:

        /**
		 * Conversion function with different block type and size
		 * @tparam OtherBlockType Type of the block to convert from
		 * @tparam OtherSize Size of the other bitset to convert from
		 * @param other Other bitset instance to convert from
		 */
        template <unsigned_integer OtherBlockType, size_t OtherSize> requires (std::convertible_to<OtherBlockType, BlockType> && !std::is_same_v<BlockType, OtherBlockType>)
        constexpr void from_other(const bitset<OtherBlockType, OtherSize>& other) noexcept
        {
            clear();
            _from_other<OtherBlockType, OtherSize>(other);
        }

        /**
         * String to bitset conversion function
         * @tparam Elem Type of the character array
         * @param str String to convert from (must be null-terminated)
         * @param set_chr Character that represents set bits
         */
        template <char_type Elem>
        constexpr void from_string(const std::basic_string<Elem>& str, const Elem& set_chr = '1') noexcept
        {
            clear();
            _from_string(str, set_chr);
        }

        /**
		 * bitset to string conversion function
		 * @tparam Elem Type of character in the array
		 * @param set_chr Character to represent set bits
		 * @param rst_chr Character to represent clear bits
		 * @return String representation of the bitset
		 */
        template <char_type Elem = char>
        [[nodiscard]] constexpr std::basic_string<Elem> to_string(const Elem& set_chr = '1', const Elem& rst_chr = '0') const /* can't use noexcept - std::basic_string */
        {
            std::basic_string<Elem> result(Size + 1, 0);

            for (size_t i = 0; i < m_storage_size - !!m_partial_size; ++i)
            {
                for (uint16_t j = 0; j < m_block_size; ++j)
                    result[i * m_block_size + j] = m_data[i] & BlockType{ 1 } << j ? set_chr : rst_chr;
            }
            if (m_storage_size)
            {
                for (size_t i = 0; i < m_partial_size; ++i)
                    result[(m_storage_size - 1) * m_block_size + i] = m_data[i] & BlockType{ 1 } << i ? set_chr : rst_chr;
            }
            return result;
        }

        /**
		 * Block stack array copy function
		 * @tparam OtherSize Size of the other bitset to copy from
		 * @param arr Array of BlockType values to initialize the bitset with (block values).
		 */
        template <size_t OtherSize>
        constexpr void from_block_array(const BlockType(&arr)[OtherSize]) noexcept
        {
            constexpr size_t min_storage_size = (std::min)(m_storage_size, OtherSize);
            for (size_t i = 0; i < min_storage_size; ++i)
                m_data[i] = arr[i];
            for (size_t i = min_storage_size; i < m_storage_size; ++i)
                m_data[i] = 0;
        }

        /**
		 * Block pointer array copy function
		 * @tparam PtrArr Type of the pointer array [don't use]
		 * @param ptr Pointer to array of BlockType values to initialize the bitset with (block values). It's size must be >= storage_size()
		 */
        template <typename PtrArr = const BlockType*> requires (std::is_pointer_v<PtrArr> && !std::is_array_v<PtrArr>&& std::is_same_v<BlockType, std::remove_cvref_t<std::remove_pointer_t<std::remove_cvref_t<PtrArr>>>> && !std::is_same_v<bool, std::remove_cvref_t<std::remove_pointer_t<std::remove_cvref_t<PtrArr>>>>)
        void from_block_array(PtrArr ptr) noexcept
        {
            for (size_t i = 0; i < m_storage_size; ++i)
                m_data[i] = *ptr++;
        }

    private:

        /**
		 * Block stack array conversion constructor with different size and block type
		 * @tparam OtherBlockType Type of the block to convert from
		 * @tparam OtherSize Size of the other bitset to convert from
		 * @param arr Array of OtherBlockType values to convert from.
		 */
        template <unsigned_integer OtherBlockType, size_t OtherSize> requires (std::convertible_to<OtherBlockType, BlockType> && !std::is_same_v<BlockType, OtherBlockType>)
        constexpr void _from_block_array(const OtherBlockType(&arr)[OtherSize]) noexcept
        {
            if (sizeof(BlockType) > sizeof(OtherBlockType))
            {
                constexpr uint16_t diff = sizeof(BlockType) / sizeof(OtherBlockType), other_block_size = sizeof(OtherBlockType) * 8;

                for (size_t i = 0; i < m_storage_size; ++i)
                {
                    for (size_t j = 0; j < diff; ++j)
                    {
                        if (i * diff + j > OtherSize)
                        {
                            return;
                        }
                        m_data[i] |= static_cast<BlockType>(arr[i * diff + j]) << j * other_block_size;
                    }
                }
            }
            else
            {
                constexpr uint16_t diff = sizeof(OtherBlockType) / sizeof(BlockType);

                for (size_t i = 0; i < OtherSize; ++i)
                {
                    for (uint16_t j = 0; j < diff; ++j)
                    {
                        if (i * diff + j > m_storage_size)
                        {
                            return;
                        }
                        m_data[i * diff + j] = arr[i] >> j % diff * m_block_size;
                    }
                }
            }
        }

        /**
		 * Block stack pointer array conversion function with different block type and size
		 * @tparam PtrArr Type of the block pointer array [don't use]
		 * @tparam Elem Type of elem in the block pointer array [don't use]
		 * @param ptr Pointer to array of BlockType values to initialize the bitset with (block values). It's size must be >= storage_size()
		 */
        template <typename PtrArr, unsigned_integer Elem = std::remove_cvref_t<std::remove_pointer_t<std::remove_cvref_t<PtrArr>>>> requires (std::convertible_to<Elem, BlockType>&& std::convertible_to<BlockType, Elem> && !std::is_same_v<BlockType, Elem>&& std::is_pointer_v<PtrArr> && !std::is_array_v<PtrArr> && !std::is_same_v<bool, Elem>)
        void _from_block_array(PtrArr const& ptr) noexcept
        {
            if (sizeof(BlockType) > sizeof(Elem))
            {
                constexpr uint16_t diff = sizeof(BlockType) / sizeof(Elem), other_block_size = sizeof(Elem) * 8;

                for (size_t i = 0; i < m_storage_size; ++i)
                {
                    for (size_t j = 0; j < diff; ++j)
                        m_data[i] |= static_cast<BlockType>(*(ptr + i * diff + j)) << j * other_block_size;
                }
            }
            else
            {
                constexpr uint16_t diff = sizeof(Elem) / sizeof(BlockType);

                for (size_t i = 0; i < m_storage_size; ++i)
                {
                    for (uint16_t j = 0; j < diff; ++j)
                        m_data[i * diff + j] = *(ptr + i) >> j % diff * m_block_size;
                }
            }
        }

    public:

        /**
		 * Block stack array conversion constructor with different size and block type
		 * @tparam OtherBlockType Type of the block to convert from
		 * @tparam OtherSize Size of the other bitset to convert from
		 * @param arr Array of OtherBlockType values to convert from.
		 */
        template <unsigned_integer OtherBlockType, size_t OtherSize> requires (std::convertible_to<OtherBlockType, BlockType> && !std::is_same_v<BlockType, OtherBlockType>)
        constexpr void from_block_array(const OtherBlockType(&arr)[OtherSize]) noexcept
        {
            clear();
            _from_block_array<OtherBlockType, OtherSize>(arr);
        }

        /**
         * Block stack pointer array conversion function with different block type and size
         * @tparam PtrArr Type of the block pointer array [don't use]
         * @tparam Elem Type of elem in the block pointer array [don't use]
         * @param ptr Pointer to array of BlockType values to initialize the bitset with (block values). It's size must be >= storage_size()
         */
        template <typename PtrArr, unsigned_integer Elem = std::remove_cvref_t<std::remove_pointer_t<std::remove_cvref_t<PtrArr>>>> requires (std::convertible_to<Elem, BlockType>&& std::convertible_to<BlockType, Elem> && !std::is_same_v<BlockType, Elem>&& std::is_pointer_v<PtrArr> && !std::is_array_v<PtrArr> && !std::is_same_v<bool, Elem>)
        void from_block_array(PtrArr const& ptr) noexcept
        {
            clear();
            _from_block_array<PtrArr, Elem>(ptr);
        }

        /**
		 * bitset to block array conversion function [or basically copy m_data]
		 * @return Array of block values (size of the array == storage_size())
		 */
        [[nodiscard]] BlockType* to_block_array() const noexcept
        {
            BlockType result = new BlockType[m_storage_size];
            std::copy(m_data, m_data + m_storage_size, result);
            return result;
        }

    private:

        /**
		 * C string stack array to bitset conversion function
		 * @tparam Elem Type of the character array
		 * @tparam StrSize Size of the character array
		 * @param c_str Array c string to fill the bitset with (must be null-terminated)
		 * @param set_chr Character that represents set bits
		 */
        template <char_type Elem, size_t StrSize>
        constexpr void _from_c_string(const Elem(&c_str)[StrSize], const Elem& set_chr = '1') noexcept
        {
            for (size_t i = 0; i < m_storage_size; ++i)
            {
                for (uint16_t j = 0; j < m_block_size; ++j)
                {
                    if (!c_str[i * m_block_size + j])
                    {
                        return;
                    }
                    m_data[i] |= c_str[i * m_block_size + j] == set_chr ? BlockType{ 1 } << j : 0;
                }
            }
        }

        /**
         * C string pointer array to bitset conversion function
         * @tparam PtrArr Type of the character array [don't use]
         * @tparam Elem Type of the character array [don't use]
         * @param c_str Pointer to c string to fill the bitset with (must be null-terminated)
         * @param set_chr Character that represents set bits
         */
        template <typename PtrArr = const char*, char_type Elem = std::remove_cvref_t<std::remove_pointer_t<std::remove_cvref_t<PtrArr>>>> requires (std::is_pointer_v<PtrArr> && !std::is_array_v<PtrArr>)
        void _from_c_string(PtrArr c_str, const Elem& set_chr = '1') noexcept
        {
            for (size_t i = 0; i < m_storage_size; ++i)
            {
                for (uint16_t j = 0; j < m_block_size; ++j)
                {
                    if (!*c_str)
                    {
                        return;
                    }
                    m_data[i] |= *c_str++ == set_chr ? BlockType{ 1 } << j : 0;
                }
            }
        }

    public:

        /**
		 * C string stack array to bitset conversion function
		 * @tparam Elem Type of the character array
		 * @tparam StrSize Size of the character array
		 * @param c_str Array c string to fill the bitset with (must be null-terminated)
		 * @param set_chr Character that represents set bits
		 */
        template <char_type Elem, size_t StrSize>
        constexpr void from_c_string(const Elem(&c_str)[StrSize], const Elem& set_chr = '1') noexcept
        {
            clear();
            _from_c_string<Elem, StrSize>(c_str, set_chr);
        }

        /**
         * C string pointer array to bitset conversion function
         * @tparam PtrArr Type of the character array [don't use]
         * @tparam Elem Type of the character array [don't use]
         * @param c_str Pointer to c string to fill the bitset with (must be null-terminated)
         * @param set_chr Character that represents set bits
         */
        template <typename PtrArr = const char*, char_type Elem = std::remove_cvref_t<std::remove_pointer_t<std::remove_cvref_t<PtrArr>>>> requires (std::is_pointer_v<PtrArr> && !std::is_array_v<PtrArr>)
        void from_c_string(PtrArr c_str, const Elem& set_chr = '1') noexcept
        {
            clear();
            _from_c_string<PtrArr, Elem>(c_str, set_chr);
        }

        /**
         * bitset to C string conversion function
         * @param set_chr Character to represent set bits
         * @param rst_chr Character to represent clear bits
         * @tparam Elem Type of character in the array
         * @return C string representation of the bitset
         */
        template <char_type Elem = char>
        [[nodiscard]] Elem* to_c_string(const Elem& set_chr = '1', const Elem& rst_chr = '0') const noexcept
        {
            Elem* result = new Elem[Size + 1];
            for (size_t i = 0; i < m_storage_size - !!m_partial_size; ++i)
            {
                for (uint16_t j = 0; j < m_block_size; ++j)
                    *(result + i * m_block_size + j) = m_data[i] & BlockType{ 1 } << j ? set_chr : rst_chr;
            }
            if (m_partial_size)
            {
                for (size_t i = 0; i < m_partial_size; ++i)
                    *(result + (m_storage_size - 1) * m_block_size + i) = m_data[i] & BlockType{ 1 } << i ? set_chr : rst_chr;
            }
            *(result + Size) = '\0';
            return result;
        }

    private:

        /**
         * Bool array to bitset conversion function
         * @tparam OtherSize Size of the bool array to copy from
         * @param arr Bool array containing the values to convert from (bit values).
         */
        template <uint64_t OtherSize>
        constexpr void _from_bool_array(const bool(&arr)[OtherSize]) noexcept
        {
            for (size_t i = 0; i < m_full_storage_size; ++i)
            {
                for (uint16_t j = 0; j < m_block_size; ++j)
                {
                    if (i * m_block_size + j >= OtherSize)
                    {
                        return;
                    }
                    m_data[i] |= arr[i * m_block_size + j] << j;
                }
            }
            if (m_partial_size)
            {
                for (uint16_t j = 0; j < m_partial_size; ++j)
                    m_data[m_storage_size - 1] |= static_cast<BlockType>(arr[m_storage_size - !!m_partial_size * m_block_size + j]) << j;
            }
        }

        /**
         * Bool pointer array to bitset conversion function
         * @tparam PtrArr Used to constrain type to const bool* [don't use]
         * @param ptr Pointer to the bool array containing the values to initialize the bitset with (bit values). It's size must be >= size()
         */
        template <typename PtrArr = const bool*> requires (std::is_pointer_v<PtrArr> && !std::is_array_v<PtrArr>&& std::is_same_v<const bool*, PtrArr> && !std::is_same_v<std::remove_cvref_t<BlockType>, bool*>)
        void _from_bool_array(PtrArr ptr) noexcept
        {
            for (size_t i = 0; i < m_full_storage_size; ++i)
            {
                for (uint16_t j = 0; j < m_block_size; ++j)
                    m_data[i] |= *ptr++ << j;
            }
            if (m_partial_size)
            {
                for (uint16_t j = 0; j < m_partial_size; ++j)
                    m_data[m_storage_size - 1] |= static_cast<BlockType>(*ptr++) << j;
            }
        }

    public:

        /**
		 * Bool array to bitset conversion function
		 * @tparam OtherSize Size of the bool array to copy from
		 * @param arr Bool array containing the values to convert from (bit values).
		 */
        template <uint64_t OtherSize>
        constexpr void from_bool_array(const bool(&arr)[OtherSize]) noexcept
        {
            clear();
            _from_bool_array<OtherSize>(arr);
        }

        /**
         * Bool pointer array to bitset conversion function
         * @tparam PtrArr Used to constrain type to const bool* [don't use]
         * @param ptr Pointer to the bool array containing the values to initialize the bitset with (bit values). It's size must be >= size()
         */
        template <typename PtrArr = const bool*> requires (std::is_pointer_v<PtrArr> && !std::is_array_v<PtrArr>&& std::is_same_v<const bool*, PtrArr> && !std::is_same_v<std::remove_cvref_t<BlockType>, bool*>)
        void from_bool_array(PtrArr ptr) noexcept
        {
            clear();
            _from_bool_array<PtrArr>(ptr);
        }

        /**
         * bitset to bool array conversion function
         * @return Pointer to array of booleans containing the bit values (size of the array == m_size template parameter)
         */
        [[nodiscard]] bool* to_bool_array() const noexcept
        {
            bool* result = new bool[Size];

            for (size_t i = 0; i < m_storage_size - !!m_partial_size; ++i)
            {
                for (uint16_t j = 0; j < m_block_size; ++j)
                    *(result + i * m_block_size + j) = m_data[i] & BlockType{ 1 } << j;
            }
            if (m_partial_size)
            {
                for (size_t i = 0; i < m_partial_size; ++i)
                    *(result + (m_storage_size - 1) * m_block_size + i) = m_data[i] & BlockType{ 1 } << i;
            }
            return result;
        }

        /**
		 * integral value to bitset conversion function
		 * @tparam T Type of the integral value to convert from
		 * @param value Value to convert from
		 */
        template <unsigned_integer T>
        constexpr void from_integer(const T& value) noexcept
        {
            clear();
            if (sizeof(T) <= sizeof(BlockType))
            {
                m_data[0] = m_data[0] & ~static_cast<BlockType>((std::numeric_limits<T>::max)()) | value;
                return;
            }

            constexpr uint16_t diff = sizeof(T) / sizeof(BlockType);

            for (uint16_t i = 0; i < diff; ++i)
            {
                m_data[i] = value >> i * m_block_size;
            }
        }

        /**
		 * bitset to integral value conversion function
		 * @tparam T Type of the integral value to convert to
		 * @return Converted integer value.
		 */
        template <unsigned_integer T>
        [[nodiscard]] constexpr T to_integer() const noexcept
        {
            if (sizeof(T) <= sizeof(BlockType))
                return static_cast<T>(m_data[0]);

            constexpr uint16_t diff = sizeof(T) / sizeof(BlockType);
            T result = 0;
            for (int16_t i = 0; i < diff; ++i)
                result |= static_cast<T>(m_data[i]) << i * m_block_size;

            return result;
        }

        // Utility functions

        /**
         * Returns current dynamic_bitset instance as const to enable optimizations (use when iterating, using [] operator, ...)
         */
        [[nodiscard]] constexpr const bitset& as_const() const noexcept
        {
            return *this;
        }

        /**
         * Swaps the values of two bits
         * @param index_1 Index of first value to swap (bit index)
         * @param index_2 Index of second value to swap (bit index)
         */
        constexpr void swap(const size_t& index_1, const size_t& index_2) noexcept
        {
            const bool tmp = test(index_1);
            set(index_1, test(index_2));
            set(index_2, tmp);
        }

        /**
         * Reverses the bitset (bit values)
         */
        constexpr void reverse() noexcept
        {
            for (size_t i = 0; i < Size / 2; ++i)
            {
                swap(i, Size - i - 1);
            }
		}

        /**
         * Rotates each bit to the left by the specified amount
         * @param shift Amount of bits to rotate to the left
         */
        constexpr void rotate(const size_t& shift) noexcept
        {
        	const bitset tmp_cpy = *this;
            for (size_t i = 0; i < Size; ++i)
                set(i, tmp_cpy.test((i + shift) % Size));
        }

        /**
         * @return Size of the bitset (bit count)
         */
        [[nodiscard]] static consteval size_t size() noexcept { return Size; }

        /**
         * @return Number of blocks in the bitset
         */
        [[nodiscard]] static consteval size_t storage_size() noexcept { return m_storage_size; }

        /**
         * @return Number of fully utilized blocks in the bitset
		 */
        [[nodiscard]] static consteval size_t full_storage_size() noexcept { return m_full_storage_size; }

        /**
         * @return Number of bits that are utilizing a block partially
		 */
		[[nodiscard]] static consteval bool partial_size() noexcept { return m_partial_size; }

        /**
         * @return Pointer to the underlying array
         */
        [[nodiscard]] constexpr BlockType*& data() noexcept { return m_data; }

        /**
         * @return Const pointer to the underlying array
         */
        [[nodiscard]] constexpr const BlockType* const& data() const noexcept { return m_data; }

        // Iteration functions

        [[nodiscard]] constexpr iterator begin() noexcept
        {
            return iterator(*this);
        }

        [[nodiscard]] constexpr iterator end() noexcept
        {
            return iterator(*this, Size);
        }

        [[nodiscard]] constexpr const_iterator begin() const noexcept
        {
            return const_iterator(*this);
        }

        [[nodiscard]] constexpr const_iterator end() const noexcept
        {
            return const_iterator(*this, Size);
        }

        [[nodiscard]] constexpr const_iterator cbegin() const noexcept
        {
            return const_iterator(*this);
        }

        [[nodiscard]] constexpr const_iterator cend() const noexcept
        {
            return const_iterator(*this, Size);
        }

        [[nodiscard]] constexpr reverse_iterator rbegin() noexcept
        {
            return reverse_iterator(*this, Size - 1);
        }

        [[nodiscard]] constexpr reverse_iterator rend() noexcept
        {
            return reverse_iterator(*this, -1);
        }

        [[nodiscard]] constexpr const_reverse_iterator crbegin() const noexcept
        {
            return const_reverse_iterator(*this);
        }

        [[nodiscard]] constexpr const_reverse_iterator crend() const noexcept
        {
            return const_reverse_iterator(*this, -1);
        }

        [[nodiscard]] constexpr const_reverse_iterator rbegin() const noexcept
        {
            return const_reverse_iterator(*this);
        }

        [[nodiscard]] constexpr const_reverse_iterator rend() const noexcept
        {
            return const_reverse_iterator(*this, -1);
        }

        // bitset operations

        /**
         * Sets the bit at the specified index to the specified value
         * @param value Value to set the bit to (bit value)
         * @param index Index of the bit to set (bit index)
         */
        constexpr void set(const size_t& index, const bool& value = true) noexcept
        {
            if (value)
                m_data[index / m_block_size] |= BlockType{1} << index % m_block_size;
            else
                m_data[index / m_block_size] &= ~(BlockType{1} << index % m_block_size);
        }

        /**
         * Sets the bit at the specified index to 0 (false)
         * @param index Index of the bit to clear (bit index)
         */
        constexpr void clear(const size_t& index) noexcept
        {
            m_data[index / m_block_size] &= ~(BlockType{1} << index % m_block_size);
        }

        /**
         * Fills all the bits with the specified value
         * @param value Value to fill the bits with (bit value)
         */
        constexpr void fill(const bool& value) noexcept
        {
            if (std::is_constant_evaluated())
            {
                for (BlockType& i : m_data)
                    i = value ? (std::numeric_limits<BlockType>::max)() : 0;
            }
            else
                ::memset(m_data, value ? (std::numeric_limits<BlockType>::max)() : 0, m_storage_size * sizeof(BlockType));
        }

        /**
         * Fills all the bits with 1 (true)
         */
        constexpr void set() noexcept
        {
            if (std::is_constant_evaluated())
            {
                for (BlockType& i : m_data)
                    i = static_cast<BlockType>(-1);
            }
            else
                ::memset(m_data, 255u, m_storage_size * sizeof(BlockType));
        }

        /**
         * Resets all the bits (sets all bits to 0)
         */
        constexpr void clear() noexcept
        {
            if (std::is_constant_evaluated())
            {
                for (BlockType& i : m_data)
                    i = 0;
            }
            else
                ::memset(m_data, 0, m_storage_size * sizeof(BlockType));
        }

        /**
         * Fills all the bits in the specified range with the specified value
         * @param value Value to fill the bits with (bit value)
         * @param end End of the range to fill (bit index)
         */
        constexpr void fill_range(const size_t& end, const bool& value) noexcept
        {
            if (std::is_constant_evaluated()) 
            {
                for (size_t i = 0; i < end / m_block_size * sizeof(BlockType); ++i)
                    m_data[i] = value ? (std::numeric_limits<BlockType>::max)() : 0;
            }
            else 
                ::memset(m_data, value ? 255u : 0, end / m_block_size * sizeof(BlockType));
            
            if (end % m_block_size)
            {
                if (value)
                {
                    for (uint16_t i = 0; i < end % m_block_size; ++i)
                        m_data[end / m_block_size] |= BlockType{1} << i;
                }
                else if (!value)
                {
                    for (uint16_t i = 0; i < end % m_block_size; ++i)
                        m_data[end / m_block_size] &= ~(BlockType{1} << i);
                }
            }
        }

        /**
         * Fills all the bits in the specified range with the specified value
         * @param end End of the range to set (bit index)
         */
        constexpr void set_range(const size_t& end) noexcept
        {
            if (std::is_constant_evaluated())
			{
				for (size_t i = 0; i < end / m_block_size * sizeof(BlockType); ++i)
					m_data[i] = (std::numeric_limits<BlockType>::max)();
			}
			else
				::memset(m_data, 255u, end / m_block_size * sizeof(BlockType));

        	if (end % m_block_size)
            {
                for (uint16_t i = 0; i < end % m_block_size; ++i)
                    m_data[end / m_block_size] |= BlockType{1} << i;
            }
        }

        /**
         * Fills all the bits in the specified range with 0 (false)
         * @param end End of the range to set (bit index)
         */
        constexpr void clear_range(const size_t& end) noexcept
        {
            if (std::is_constant_evaluated())
            {
	            for (size_t i = 0; i < end / m_block_size * sizeof(BlockType); ++i)
					m_data[i] = 0;
			}
			else
				::memset(m_data, 0, end / m_block_size * sizeof(BlockType));

        	if (end % m_block_size)
            {
                for (uint16_t i = 0; i < end % m_block_size; ++i)
                    m_data[end / m_block_size] &= ~(BlockType{1} << i);
            }
        }

        /**
         * Fills all the bits in the specified range with the specified value
         * @param value Value to fill the bits with (bit value)
         * @param begin Begin of the range to fill (bit index)
         * @param end End of the range to fill (bit index)
         */
        constexpr void fill_range(const size_t& begin, const size_t& end, const bool& value) noexcept
        {
            uint8_t to_add = 1, to_sub = 1;
            // create begin_block and fill the first byte with it
            if (begin % m_block_size)
            {
                const uint16_t end_bit = (begin / m_block_size == end / m_block_size) ? end % m_block_size : m_block_size;
                if (value)
                {
                    for (uint16_t i = begin % m_block_size; i < end_bit; ++i)
                        m_data[begin / m_block_size] |= BlockType{1} << i;
                }
                else
                {
                    for (uint16_t i = begin % m_block_size; i < end_bit; ++i)
                        m_data[begin / m_block_size] &= ~(BlockType{1} << i);
                }
            }
            else
                to_add = 0;

            // set the end block if the end is not aligned with the block size
            if (end % m_block_size && begin / m_block_size != end / m_block_size)
            {
                if (value)
                {
                    for (uint16_t i = 0; i < end % m_block_size; ++i)
                        m_data[end / m_block_size] |= BlockType{1} << i;
                }
                else
                {
                    for (uint16_t i = 0; i < end % m_block_size; ++i)
                        m_data[end / m_block_size] &= ~(BlockType{1} << i);
                }
            }
            else
                to_sub = 0;

            if (std::is_constant_evaluated())
			{
				for (size_t i = begin / m_block_size + to_add; i < end / m_block_size; ++i)
					m_data[i] = value ? (std::numeric_limits<BlockType>::max)() : 0;
			}
			else
				::memset(m_data + begin / m_block_size + to_add, value ? 255u : 0, (end - begin) / m_block_size * sizeof(BlockType) - to_sub);
        }

        /**
         * Fills all the bits in the specified range with 0 (false)
         * @param begin Begin of the range to fill (bit index)
         * @param end End of the range to fill (bit index)
         */
        constexpr void set_range(const size_t& begin, const size_t& end) noexcept
        {
            uint8_t to_add = 1, to_sub = 1;
            // create begin_block and fill the first byte with it
            if (begin % m_block_size)
            {
                const uint16_t end_bit = begin / m_block_size == end / m_block_size ? end % m_block_size : m_block_size;
                for (uint16_t i = begin % m_block_size; i < end_bit; ++i)
                    m_data[begin / m_block_size] |= BlockType{1} << i;
            }
            else
                to_add = 0;

            // set the end block if the end is not aligned with the block size
            if (end % m_block_size && begin / m_block_size != end / m_block_size)
            {
                for (uint16_t i = 0; i < end % m_block_size; ++i)
                    m_data[end / m_block_size] |= BlockType{1} << i;
            }
            else
                to_sub = 0;
            if (std::is_constant_evaluated())
            {
	            for (size_t i = begin / m_block_size + to_add; i < end / m_block_size; ++i)
					m_data[i] = (std::numeric_limits<BlockType>::max)();
			}
			else
				::memset(m_data + begin / m_block_size + to_add, 255u, (end - begin) / m_block_size * sizeof(BlockType) - to_sub);
        }

        /**
         * Fills all the bits in the specified range with 0 (false)
         * @param begin Begin of the range to fill (bit index)
         * @param end End of the range to fill (bit index)
         */
        constexpr void clear_range(const size_t& begin, const size_t& end) noexcept
        {
            uint8_t to_add = 1, to_sub = 1;
            // create begin_block and fill the first byte with it
            if (begin % m_block_size)
            {
                const uint16_t end_bit = begin / m_block_size == end / m_block_size ? end % m_block_size : m_block_size;
                for (uint16_t i = begin % m_block_size; i < end_bit; ++i)
                    m_data[begin / m_block_size] &= ~(BlockType{1} << i);
            }
            else
                to_add = 0;

            // set the end block if the end is not aligned with the block size
            if (end % m_block_size && begin / m_block_size != end / m_block_size)
            {
                for (uint16_t i = 0; i < end % m_block_size; ++i)
                    m_data[end / m_block_size] &= ~(BlockType{1} << i);
            }
            else
                to_sub = 0;

            if (std::is_constant_evaluated())
			{
				for (size_t i = begin / m_block_size + to_add; i < end / m_block_size; ++i)
					m_data[i] = 0;
			}
			else
				::memset(m_data + begin / m_block_size + to_add, 0, (end - begin) / m_block_size * sizeof(BlockType) - to_sub);
        }

        /**
         * Fills all the bits in the specified range with the specified value
         * @param value Value to fill the bits with (bit value)
         * @param begin Begin of the range to fill (bit index)
         * @param end End of the range to fill (bit index)
         * @param step Step size between the bits to fill (bit step)
         */
        constexpr void fill_range(const size_t& begin, const size_t& end, const size_t& step, const bool& value) noexcept
        {
            for (size_t i = begin; i < end; i += step)
            {
                if (value)
                    m_data[i / m_block_size] |= BlockType{1} << i % m_block_size;
                else
                    m_data[i / m_block_size] &= ~(BlockType{1} << i % m_block_size);
            }
        }

        /**
         * Fills all the bits in the specified range with 0 (false)
         * @param begin Begin of the range to fill (bit index)
         * @param end End of the range to fill (bit index)
         * @param step Step size between the bits to fill (bit step)
         */
        constexpr void set_range(const size_t& begin, const size_t& end, const size_t& step) noexcept
        {
            for (size_t i = begin; i < end; i += step)
                m_data[i / m_block_size] |= BlockType{1} << i % m_block_size;
        }

        /**
         * Fills all the bits in the specified range with 0 (false)
         * @param begin Begin of the range to fill (bit index)
         * @param end End of the range to fill (bit index)
         * @param step Step size between the bits to fill (bit step)
         */
        constexpr void clear_range(const size_t& begin, const size_t& end, const size_t& step) noexcept
        {
            for (size_t i = begin; i < end; i += step)
                m_data[i / m_block_size] &= ~(BlockType{1} << i % m_block_size);
        }

        /**
         * !!! W.I.P. - Does not function correctly at the moment !!!\n
         * Fill the bits in the specified range with the specified value using an optimized algorithm.\n
         * This algorithm is particularly efficient when the step size is relatively low.\n
         * Note: This function has a rather complex implementation. It is not recommended to use it when simple filling without a step is possible.\n
         * Performance of this function varies significantly depending on the step. It performs best when step is a multiple of m_block_size, and is within reasonable range from it.\n
         * However, worst when step is not aligned with m_block_size and end is not aligned with m_block_size. In such cases, extra processing is required to handle the boundary blocks.\n
         * @param value Value to fill the bits with (bit value)
         * @param begin Begin of the range to fill (bit index)
         * @param end End of the range to fill (bit index)
         * @param step Step size between the bits to fill (bit step)
         */
        constexpr void fill_range_optimized(const size_t& begin, const size_t& end, const size_t& step, const bool& value) noexcept
        {
            // Initialize variables
            size_t blocks_size, current_block = begin / m_block_size + 1 + step / m_block_size, current_offset = 0;
            uint16_t offset;
            const size_t end_block = end / m_block_size + (end % m_block_size ? 1 : 0);

            // Determine the size of blocks based on step and block size
            if ((step % 2 || step <= m_block_size) && m_block_size % step) {
                blocks_size = (std::min)(m_storage_size, step);
            }
            else if (!(m_block_size % step))
                blocks_size = 1;
            else if (!(step % m_block_size))
                blocks_size = step / m_block_size;
            else
            {
                // GCD of step and m_block_size
                if (step % m_block_size)
                {
                    size_t a = step, b = m_block_size, t = m_block_size;
                    while (b) {
                        t = b;
                        b = a % b;
                        a = t;
                    }
                    blocks_size = a;
                }
                else
                    blocks_size = 1;
            }

            // Calculate the offset
            if (begin < m_block_size)
            {
                offset = (m_block_size - begin) % step;
                if (offset)
                    offset = step - offset;
            }
            else
            {
                offset = (begin - m_block_size) % step;
            }

            if (offset)
                offset = (m_block_size - offset + step / m_block_size * m_block_size) % step;

            std::cout << "offset: " << offset << '\n';

            // Create and apply the beginning block
            {
                const uint16_t end_bit = (begin / m_block_size == end / m_block_size) ? end % m_block_size : m_block_size;
                if (value)
                {
                    for (uint16_t i = begin % m_block_size; i < end_bit; i += step)
                        m_data[begin / m_block_size] |= BlockType{1} << i;
                }
                else
                {
                    for (uint16_t i = begin % m_block_size; i < end_bit; i += step)
                        m_data[begin / m_block_size] &= ~(BlockType{1} << i);
                }
            }


            // Fill with appropriate block
            std::cout << blocks_size << " blocks\n";
            std::cout << (std::min)(blocks_size + begin / m_block_size, m_storage_size) << " blocks\n";
            for (size_t i = 0; i < (std::min)(blocks_size, m_storage_size); ++i)
            {
                // Generate block for the current iteration
                BlockType block = 0;

                if (value)
                {
                    for (uint16_t j = !i ? offset : 0; j < m_block_size; ++j)
                    {
                        std::cout << current_block * m_block_size + j - offset << '\n';
                        if (!((current_block * m_block_size + j - offset) % step))
                            block |= BlockType{1} << j;
                    }
                }
                else
                {
                    block = (std::numeric_limits<BlockType>::max)();
                    for (uint16_t j = (!i ? offset : 0); j < m_block_size; ++j)
                    {
                        if (!((current_block * m_block_size + j - offset) % step))
                            block &= ~(BlockType{1} << j);
                    }
                }

                // print the block
                for (uint16_t j = 0; j < m_block_size; ++j)
                {
                    std::cout << ((block & BlockType{1} << j) >> j);
                }

                std::cout << '\n';

                // Apply the block
                for (size_t j = current_block; j < m_storage_size; ++j)
                {
                    if (j == end_block - 1 && end % m_block_size)
                    {
                        // Remove bits that overflow the range
                        if (value)
                        {
                            for (uint16_t k = end % m_block_size; k < m_block_size; ++k)
                                block &= ~(BlockType{1} << k);
                            *(m_data + j) |= block;
                        }
                        else
                        {
                            for (uint16_t k = end % m_block_size; k < m_block_size; ++k)
                                block |= BlockType{1} << k;
                            *(m_data + j) &= block;
                        }
                        break;
                    }
                    if (value)
                        *(m_data + j) |= block;
                    else
                        *(m_data + j) &= block;
                }
                ++current_block;
            }
        }

        /**
         * !!! W.I.P. - May not choose the best option, not even talking about the fact that set_range_optimized function doesn't even work correctly !!!\n
         * Fill the bits in the specified range with the specified value.\n
         * Chooses the fastest implementation based on the step.\n
         * This function becomes more accurate in choosing the fastest implementation as the size of the bitset increases.\n
         * @param value Value to fill the bits with
         * @param begin Begin of the range to fill (bit index)
         * @param end End of the range to fill (bit index)
         * @param step Step size between the bits to fill
        */
        constexpr void fill_range_fastest(const size_t& begin, const size_t& end, const size_t& step, const bool& value) noexcept
        {
            if (step == 1)
            {
                fill_range(begin, end, value);
                return;
            }
            if (step <= m_block_size * 2.5) // approximately up until this point it is faster, though no scientific anything went into this, just a guess lol
            {
                fill_range_optimized(value, begin, end, step);
                return;
            }
            fill_range(value, begin, end, step);
        }

        /**
         * Sets the block at the specified index to the specified value (default is max value, all bits set to 1)
         * @param block Chunk to set (block value)
         * @param index Index of the block to set (block index)
         */
        constexpr void set_block(const size_t& index, const BlockType& block = (std::numeric_limits<BlockType>::max)()) noexcept
        {
            m_data[index] = block;
        }

        /**
         * Sets the block at the specified index to 0 (all bits set to 0)
         * @param index Index of the block to clear (block index)
         */
        constexpr void clear_block(const size_t& index) noexcept
        {
            m_data[index] = 0u;
        }

        /**
         * Fills all the blocks with the specified block
         * @param block Chunk to fill the blocks with (block value)
         */
        constexpr void fill_block(const BlockType& block) noexcept
        {
            for (size_t i = 0; i < m_storage_size; ++i)
                m_data[i] = block;
        }

        /**
         * Fills all the bits in the specified range with the specified block
         * @param block Chunk to fill the bits with (block value)
         * @param end End of the range to fill (block index)
         */
        constexpr void fill_block_range(const size_t& end, const BlockType& block = (std::numeric_limits<BlockType>::max)()) noexcept
        {
            for (size_t i = 0; i < end; ++i)
                m_data[i] = block;
        }

        /**
         * Fills all the bits in the specified range with the specified block
         * @param block Chunk to fill the bits with
         * @param begin begin of the range to fill (block index)
         * @param end End of the range to fill (block index)
         */
        constexpr void fill_block_range(const size_t& begin, const size_t& end, const BlockType& block = (std::numeric_limits<BlockType>::max)()) noexcept
        {
            for (size_t i = begin; i < end; ++i)
                m_data[i] = block;
        }

        /**
         * Fills all the bits in the specified range with the specified block
         * @param block Chunk to fill the bits with (block value)
         * @param begin begin of the range to fill (block index)
         * @param end End of the range to fill (block index)
         * @param step Step size between the bits to fill (block step)
         */
        constexpr void fill_block_range(const size_t& begin, const size_t& end, const size_t& step, const BlockType& block = (std::numeric_limits<BlockType>::max)()) noexcept
        {
            for (size_t i = begin; i < end; i += step)
                m_data[i] = block;
        }

        /**
         * Flips the bit at the specified index
         * @param index Index of the bit to flip (bit index)
         */
        constexpr void flip(const size_t& index) noexcept
        {
            m_data[index / m_block_size] ^= BlockType{1} << index % m_block_size;
        }

        /**
         * Flips all the bits
         */
        constexpr void flip() noexcept
        {
            for (size_t i = 0; i < m_storage_size; ++i)
                m_data[i] = ~m_data[i];
        }

        /**
         * Flips all the bits in the specified range
         * @param end End of the range to fill (bit index)
         */
        constexpr void flip_range(const size_t& end) noexcept
        {
            // flip blocks that are in range by bulk, rest flip normally
            for (size_t i = 0; i < end / m_block_size; ++i)
                m_data[i] = ~m_data[i];
            for (uint16_t i = 0; i < end % m_block_size; ++i)
                m_data[end / m_block_size] ^= BlockType{1} << i;
        }

        /**
         * Flip all the bits in the specified range
         * @param begin Begin of the range to fill (bit index)
         * @param end End of the range to fill (bit index)
         */
        constexpr void flip_range(const size_t& begin, const size_t& end) noexcept
        {
            size_t to_add = 1;
            if (begin % m_block_size)
            {
                for (uint16_t i = begin % m_block_size; i < m_block_size; ++i)
                    m_data[begin / m_block_size] ^= BlockType{1} << i;
            }
            else
                to_add = 0;

            for (size_t i = begin / m_block_size + to_add; i < end / m_block_size; ++i)
                m_data[i] = ~m_data[i];

            if (end % m_block_size)
            {
                for (uint16_t i = 0; i < end % m_block_size; ++i)
                    m_data[end / m_block_size] ^= BlockType{1} << i;
            }
        }

        /**
         * Flips all the bits in the specified range
         * @param begin Begin of the range to fill (bit index)
         * @param end End of the range to fill (bit index)
         * @param step Step size between the bits to flip (bit step)
         */
        constexpr void flip_range(const size_t& begin, const size_t& end, const size_t& step) noexcept
        {
            for (size_t i = begin; i < end; i += step)
            {
                m_data[i / m_block_size] ^= BlockType{1} << i % m_block_size;
            }
        }

        /**
         * Flips the block at the specified index
         * @param index Index of the block to flip (block index)
         */
        constexpr void flip_block(const size_t& index) noexcept
        {
            m_data[index] = ~m_data[index];
        }

        /**
         * Flips all the blocks in the specified range
         * @param end End of the range to fill (block index)
         */
        constexpr void flip_block_range(const size_t& end) noexcept
        {
            for (size_t i = 0; i < end; ++i)
                m_data[i] = ~m_data[i];
        }

        /**
         * Flips all the blocks in the specified range
         * @param begin Begin of the range to fill (block index)
         * @param end End of the range to fill (block index)
         */
        constexpr void flip_block_range(const size_t& begin, const size_t& end) noexcept
        {
            for (size_t i = begin; i < end; ++i)
                m_data[i] = ~m_data[i];
        }

        /**
         * Flips all the blocks in the specified range
         * @param begin Begin of the range to fill (block index)
         * @param end End of the range to fill (block index)
         * @param step Step size between the bits to flip (block step)
         */
        constexpr void flip_block_range(const size_t& begin, const size_t& end, const size_t& step) noexcept
        {
            for (size_t i = begin; i < end; i += step)
                m_data[i] = ~m_data[i];
        }

        /**
         * Retrieves the value of a bit at a specified index
         * @param index The index of the bit to read (bit index)
         * @return The value of the bit at the specified index
         */
        [[nodiscard]] constexpr bool test(const size_t& index) const noexcept
        {
            return m_data[index / m_block_size] & BlockType{1} << index % m_block_size;
        }

        /**
         * Retrieves the block at the specified index
         * @param index Index of the block to retrieve (block index)
         * @return Chunk at the specified index
         */
        [[nodiscard]] constexpr const BlockType& get_block(const size_t& index) const noexcept
        {
            return m_data[index];
        }

        /**
         * Retrieves the block at the specified index
         * @param index Index of the block to retrieve (block index)
         * @return Chunk at the specified index
         */
        [[nodiscard]] constexpr BlockType& get_block(const size_t& index) noexcept
        {
            return m_data[index];
        }


        /**
         * Checks if all bits are set
         * @return true if all bits are set, false otherwise
         */
        [[nodiscard]] constexpr bool all() const noexcept
        {
            // check all except the last one if the size is not divisible by m_block_size
            for (size_t i = 0; i < m_storage_size - !!m_partial_size; ++i)
            {
                if (m_data[i] != (std::numeric_limits<BlockType>::max)())
                    return false;
            }
            if (m_partial_size)
            {
                for (uint16_t i = 0; i < m_partial_size; ++i)
                {
                    if (!(m_data[Size / m_block_size] & BlockType{1} << i))
                        return false;
                }
            }
            return true;
        }

        /**
         * Checks if any bit is set
         * @return true if any bit is set, false otherwise
         */
        [[nodiscard]] constexpr bool any() const noexcept
        {
            for (size_t i = 0; i < m_storage_size - !!m_partial_size; ++i)
            {
                if (m_data[i])
                    return true;
            }
            if (m_partial_size)
            {
                for (uint16_t i = 0; i < m_partial_size; ++i)
                {
                    if (m_data[Size / m_block_size] & BlockType{1} << i)
                        return true;
                }
            }
            return false;
        }

        /**
         * Checks if none of the bits are set
         * @return true if none of the bits are set, false otherwise
         */
        [[nodiscard]] constexpr bool none() const noexcept
        {
            for (size_t i = 0; i < m_storage_size - !!m_partial_size; ++i)
            {
                if (m_data[i])
                    return false;
            }
            if (m_partial_size)
            {
                for (uint16_t i = 0; i < m_partial_size; ++i)
                {
                    if (m_data[Size / m_block_size] & BlockType{1} << i)
                        return false;
                }
            }
            return true;
        }

        /**
         * Checks if all bits are cleared (none are set)
         * @return true if all bits are cleared, false otherwise
         */
        [[nodiscard]] constexpr bool all_clear() const noexcept
        {
            return none();
        }

        /**
         * @return The number of set bits
         */
        [[nodiscard]] constexpr size_t count() const noexcept
        {
            size_t count = 0;
            for (size_t i = 0; i < m_storage_size; ++i)
            {
                BlockType j = m_data[i];
                while (j)
                {
                    j &= j - 1;
                    ++count;
                }
            }
            return count;
        }

        /**
         * Checks if the bitset is empty
         * @return true if the bitset is empty, false otherwise
         */
        [[nodiscard]] static consteval bool empty() noexcept
        {
            return !Size;
        }

        /**
		 * Bit-length of the underlying type
		 */
        static constexpr uint16_t m_block_size = sizeof(BlockType) * 8;

        /**
		 * Size of the bitset in blocks, without last potential dangling block
		 */
        static constexpr size_t m_full_storage_size = Size / m_block_size;

        /**
         * Count of bits that are utilized in last dangling block, if any
         */
        static constexpr uint16_t m_partial_size = Size % m_block_size;

        /**
         * Size of the bitset in blocks
         */
        static constexpr size_t m_storage_size = Size / m_block_size + !!m_partial_size;

        /**
		 * Underlying array of blocks containing the bits
		 */
        alignas(64) BlockType m_data[m_storage_size];
    };

    /**
	 * Dynamic-size BitSet class
	 * Naming used across documentaion:
	 * bit value: may be either 1 (true) or 0 (false)
	 * bit index: is index of a value 1 (true) or 0 (false) in the bitset, e.g. BlockType=size_t, m_size=100, bit index is value between (0-99)
	 * bit size/count: size in bits. e.g. 1 uint32_t holds 32 bits = 32 bit size/count
	 * block value: Is BlockType value, used to directly copy bits inside it. e.g. when BlockType=uint8_t, block value may be 0b10101010 (Visible order of bits is reversed order of the actual bits)
	 * block index: index of BlockType value, e.g. when BlockType=size_t, m_size=128, block index is value between (0-1), because 128 bits fit into 2 size_t's
	 * @tparam BlockType Type of block to use for bit storage
	 */
    template <unsigned_integer BlockType>
    class dynamic_bitset
    {
    public:

        // Type definitions

        // Reference types
        class reference;
        typedef bool const_reference;

        // Iterators
        class iterator;
        class const_iterator;

        // Reverse iterators
        class reverse_iterator;
        class const_reverse_iterator;

        // Value types
        typedef bool value_type;
        typedef bool const_value_type;

        // Other types
        typedef std::size_t size_t;
        typedef size_t difference_type;
        typedef size_t size_type;
        typedef BlockType block_type;

        class reference
        {
        public:
            /**
             * Empty constructor (only here to allow for some concepts to be executed)
             */
            reference() noexcept : m_bitset(dynamic_bitset<uint8_t>(8, 0b11111111)), m_index(0) {}

            /**
             * Constructs a new reference instance based on the specified bitset and index
             * @param bitset The bitset instance to iterate over
             * @param index Index of the bit to start the iteration from (bit index)
             */
            reference(dynamic_bitset& bitset, const size_t& index) noexcept : m_bitset(bitset), m_index(index) {}

            /**
             * Copy constructor
             */
            reference(const reference&) noexcept = default;

            /**
             * Destructor
             */
            ~reference() noexcept = default;

            /**
             * Bool conversion operator
             * @return Value at current index (bit value)
             */
            [[nodiscard]] operator bool() const noexcept
            {
                return m_bitset.test(m_index);
            }

            /**
             * Assignment operator
             * @param value Value to set the bit to (bit value)
             */
            reference& operator=(const bool& value) noexcept
            {
                m_bitset.set(m_index, value);
                return *this;
            }

            /**
             * Assignment operator
             * @param other Other reference instance to copy from
             */
            reference& operator=(const reference& other) noexcept
            {
                m_bitset.set(m_index, other.m_bitset.test(other.m_index));
                return *this;
            }

            /**
             * Bitwise AND assignment operator
             * @param value Value to AND the bit with (bit value)
             */
            reference& operator&=(const bool& value) const noexcept
            {
                m_bitset.set(m_index, m_bitset.test(m_index) & value);
                return *this;
            }

            /**
             * Bitwise OR assignment operator
             * @param value Value to OR the bit with (bit value)
             */
            reference& operator|=(const bool& value) const noexcept
            {
                m_bitset.set(m_index, m_bitset.test(m_index) | value);
                return *this;
            }

            /**
             * Bitwise XOR assignment operator
             * @param value Value to XOR the bit with (bit value)
             */
            reference& operator^=(const bool& value) const noexcept
            {
                m_bitset.set(m_index, m_bitset.test(m_index) ^ value);
                return *this;
            }

            /**
             * Flip the bit at the current index
             */
            void flip() const noexcept
            {
                m_bitset.flip(m_index);
                return *this;
            }

            /**
             * Set the bit at the current index to value
             * @param value Value to set the bit to (bit value)
             */
            void set(const bool& value = true) const noexcept
            {
                m_bitset.set(m_index, value);
            }

            /**
             * Reset the bit at the current index to value
             */
            void clear() const noexcept
            {
                m_bitset.clear(m_index);
            }

            dynamic_bitset& m_bitset;
            const size_t& m_index;
        };

        /**
         * Base iterator class template.
         * @tparam BitSetTypeSpecifier Full type of the bitset to iterate over. [bitset&] or [const bitset&]
         */
        template <typename BitSetTypeSpecifier> requires std::is_same_v<dynamic_bitset, std::remove_cvref_t<BitSetTypeSpecifier>>
        class iterator_base {
        public:
            iterator_base() noexcept = delete;

            /**
             * Constructs a new iterator_base instance based on the specified bitset and index.
             * @param bitset The bitset instance to iterate over.
             * @param index Index of the bit to start the iteration from (bit index).
             */
            explicit iterator_base(BitSetTypeSpecifier bitset, const size_t& index = 0) noexcept : m_bitset(bitset), m_index(index) {}

            /**
             * Copy constructor.
             * @param other Other iterator_base instance to copy from.
             */
            iterator_base(const iterator_base& other) noexcept : m_bitset(other.m_bitset), m_index(other.m_index) {}

            /**
             * Destructor.
             */
            ~iterator_base() noexcept = default;

            /**
             * Copy assignment operator.
             */
            iterator_base& operator=(const iterator_base& other) noexcept {
                m_bitset = other.m_bitset;
                m_index = other.m_index;
                return *this;
            }

            BitSetTypeSpecifier m_bitset;
            size_t m_index;
        };

        /**
         * Iterator class.
         */
        class iterator : public iterator_base<dynamic_bitset&> {
        public:
            using iterator_base<dynamic_bitset&>::iterator_base;

            /**
             * Conversion constructor to const_iterator.
             * @param other Other const_iterator instance to convert from.
             */
            iterator(const const_iterator& other) noexcept : iterator_base<dynamic_bitset&>(other.m_bitset, other.m_index) {}

            /**
             * Conversion assign operator to const_iterator.
             * @param other Other const_iterator instance to convert from.
             */
            iterator& operator=(const const_iterator& other) noexcept {
                this->m_bitset = other.m_bitset;
                this->m_index = other.m_index;
                return *this;
            }

            /**
             * Dereference operator (returns the value of the bit at the current bit index).
             * @return Reference to the bit at the current index.
             */
            [[nodiscard]] reference operator*() noexcept {
                return reference(this->m_bitset, this->m_index);
            }

            /**
             * Increments the iterator to the next bit, pre-increment.
             */
            iterator& operator++() noexcept
            {
                ++this->m_index;
                return *this;
            }

            /**
             * Increments the iterator to the next bit, post-increment
             */
            iterator& operator++(int) noexcept
            {
                iterator tmp = *this;
                ++this->m_index;
                return tmp;
            }

            /**
             * Decrements the iterator to the previous bit, pre-decrement.
             */
            iterator& operator--() noexcept
            {
                --this->m_index;
                return *this;
            }

            /**
             * Decrements the iterator to the previous bit, post-decrement.
             */
            iterator& operator--(int) noexcept
            {
                iterator tmp = *this;
                --this->m_index;
                return tmp;
            }

            /**
             * Returns the iterator with the specified amount added to it
             * @param amount Amount to add to the iterator (bit count)
             * @return New iterator instance with the specified amount added to it
             */
            [[nodiscard]] iterator operator+(const size_t& amount) const noexcept
            {
                return iterator(this->m_bitset, this->m_index + amount);
            }

            /**
             * Returns the iterator with the specified amount subtracted from it
             * @param amount Amount to subtract from the iterator (bit count)
             * @return New iterator instance with the specified amount subtracted from it
             */
            [[nodiscard]] iterator operator-(const size_t& amount) const noexcept
            {
                return iterator(this->m_bitset, this->m_index - amount);
            }

            /**
             * Returns difference between two iterators
             * @param other Other iterator instance subtract with
             * @return Difference between two iterators
             */
            [[nodiscard]] difference_type operator-(const iterator& other) const noexcept
            {
                return this->m_index - other.m_index;
            }

            /**
             * Returns difference between two iterators
             * @param other Other iterator instance subtract with
             * @return Difference between two iterators
             */
            [[nodiscard]] difference_type operator-(const const_iterator& other) const noexcept
            {
                return this->m_index - other.m_index;
            }

            /**
             * Returns the iterator multiplied by the specified amount
             * @param amount Amount to multiply the iterator by (bit count ?)
             * @return New iterator instance multiplied by the specified amount
             */
            [[nodiscard]] iterator operator*(const size_t& amount) const noexcept
            {
                return iterator(this->m_bitset, this->m_index * amount);
            }


            /**
             * Returns the iterator divided by the specified amount
             * @param amount Amount to multiply the iterator by (bit count ?)
             * @return New iterator instance divided by the specified amount
             */
            [[nodiscard]] iterator operator/(const size_t& amount) const noexcept
            {
                return iterator(this->m_bitset, this->m_index / amount);
            }

            /**
             * Adds the specified amount to the iterator
             * @param amount Amount to add to the iterator (bit count)
             */
            iterator& operator+=(const size_t& amount) noexcept
            {
                this->m_index += amount;
                return *this;
            }

            /**
             * Subtracts the specified amount to the iterator
             * @param amount Amount to add to the iterator (bit count)
             */
            iterator& operator-=(const size_t& amount) noexcept
            {
                this->m_index -= amount;
                return *this;
            }

            /**
             * Adds the specified amount to the iterator
             * @param amount Amount to add to the iterator (bit count)
             */
            iterator& operator*=(const size_t& amount) noexcept
            {
                this->m_index *= amount;
                return *this;
            }

            /**
             * Subtracts the specified amount to the iterator
             * @param amount Amount to add to the iterator (bit count)
             */
            iterator& operator/=(const size_t& amount) noexcept
            {
                this->m_index /= amount;
                return *this;
            }

            /**
             * Inequality operator
             * @param other Other iterator instance to compare with
             * @return true if the iterators are not equal, false otherwise
             */
            [[nodiscard]] bool operator!=(const iterator& other) const noexcept
            {
                return this->m_index != other.m_index;
            }

            /**
             *  Less than operator
             *  @param other Other iterator instance to compare with
             *  @return true if the current iterator is less than the other iterator, false otherwise
             */
            [[nodiscard]] bool operator<(const iterator& other) const noexcept
            {
                return this->m_index < other.m_index;
            }

            /**
             *  Less than or equal operator
             *  @param other Other iterator instance to compare with
             *  @return true if the current iterator is less than or equal to the other iterator, false otherwise
             */
            [[nodiscard]] bool operator<=(const iterator& other) const noexcept
            {
                return this->m_index <= other.m_index;
            }

            /**
             *  Equality operator
             *  @param other Other iterator instance to compare with
             *  @return true if the iterators are equal, false otherwise
             */
            [[nodiscard]] bool operator==(const iterator& other) const noexcept
            {
                return this->m_index == other.m_index;
            }

            /**
             *  Bigger than or equal operator
             *  @param other Other iterator instance to compare with
             *  @return true if the current iterator is bigger than or equal to the other iterator, false otherwise
             */
            [[nodiscard]] bool operator>=(const iterator& other) const noexcept
            {
                return this->m_index >= other.m_index;
            }

            /**
             *  Bigger than operator
             *  @param other Other iterator instance to compare with
             *  @return true if the current iterator is bigger than the other iterator, false otherwise
             */
            [[nodiscard]] bool operator>(const iterator& other) const noexcept
            {
                return this->m_index > other.m_index;
            }
        };

        /**
         * Const iterator class
         */
        class const_iterator : public iterator_base<const dynamic_bitset&>
        {
        public:
            using iterator_base<const dynamic_bitset&>::iterator_base;

            /**
             * Dereference operator (returns the value of the bit at the current bit index)
             * @return Reference to the bit at the current index
             */
            [[nodiscard]] const_reference operator*() const noexcept
            {
                return this->m_bitset.test(this->m_index);
            }

            /**
             * Increments the iterator to the next bit, pre-increment
             */
            const_iterator& operator++() noexcept
            {
                ++this->m_index;
                return *this;
            }

            /**
             * Increments the iterator to the next bit, post-increment
             */
            const_iterator& operator++(int) noexcept
            {
                const_iterator tmp = *this;
                ++this->m_index;
                return tmp;
            }

            /**
             * Decrements the iterator to the previous bit, pre-decrement
             */
            const_iterator& operator--() noexcept
            {
                --this->m_index;
                return *this;
            }

            /**
             * Decrements the iterator to the previous bit, post-decrement
             */
            const_iterator& operator--(int) noexcept
            {
                const_iterator tmp = *this;
                --this->m_index;
                return tmp;
            }

            /**
             * Returns the iterator with the specified amount added to it
             * @param amount Amount to add to the iterator (bit count)
             * @return New iterator instance with the specified amount added to it
             */
            [[nodiscard]] const_iterator operator+(const size_t& amount) const noexcept
            {
                return const_iterator(this->m_bitset, this->m_index + amount);
            }

            /**
             * Returns the iterator with the specified amount subtracted from it
             * @param amount Amount to subtract from the iterator (bit count)
             * @return New iterator instance with the specified amount subtracted from it
             */
            [[nodiscard]] const_iterator operator-(const size_t& amount) const noexcept
            {
                return const_iterator(this->m_bitset, this->m_index - amount);
            }

            /**
             * Returns difference between two iterators
             * @param other Other iterator instance subtract with
             * @return Difference between two iterators
             */
            [[nodiscard]] difference_type operator-(const const_iterator& other) const noexcept
            {
                return this->m_index - other.m_index;
            }

            /**
             * Returns difference between two iterators
             * @param other Other iterator instance subtract with
             * @return Difference between two iterators
             */
            [[nodiscard]] difference_type operator-(const iterator& other) const noexcept
            {
                return this->m_index - other.m_index;
            }

            /**
             * Returns the iterator multiplied by the specified amount
             * @param amount Amount to multiply the iterator by (bit count ?)
             * @return New iterator instance multiplied by the specified amount
             */
            [[nodiscard]] const_iterator operator*(const size_t& amount) const noexcept
            {
                return const_iterator(this->m_bitset, this->m_index * amount);
            }


            /**
             * Returns the iterator divided by the specified amount
             * @param amount Amount to multiply the iterator by (bit count ?)
             * @return New iterator instance divided by the specified amount
             */
            [[nodiscard]] const_iterator operator/(const size_t& amount) const noexcept
            {
                return const_iterator(this->m_bitset, this->m_index / amount);
            }

            /**
             * Adds the specified amount to the iterator
             * @param amount Amount to add to the iterator (bit count)
             */
            const_iterator& operator+=(const size_t& amount) noexcept
            {
                this->m_index += amount;
                return *this;
            }

            /**
             * Subtracts the specified amount to the iterator
             * @param amount Amount to add to the iterator (bit count)
             */
            const_iterator& operator-=(const size_t& amount) noexcept
            {
                this->m_index -= amount;
                return *this;
            }

            /**
             * Adds the specified amount to the iterator
             * @param amount Amount to add to the iterator (bit count)
             */
            const_iterator& operator*=(const size_t& amount) noexcept
            {
                this->m_index *= amount;
                return *this;
            }

            /**
             * Subtracts the specified amount to the iterator
             * @param amount Amount to add to the iterator (bit count)
             */
            const_iterator& operator/=(const size_t& amount) noexcept
            {
                this->m_index /= amount;
                return *this;
            }

            /**
             * Inequality operator
             * @param other Other iterator instance to compare with
             * @return true if the iterators are not equal, false otherwise
             */
            [[nodiscard]] bool operator!=(const const_iterator& other) const noexcept
            {
                return this->m_index != other.m_index;
            }

            /**
             *  Less than operator
             *  @param other Other iterator instance to compare with
             *  @return true if the current iterator is less than the other iterator, false otherwise
             */
            [[nodiscard]] bool operator<(const const_iterator& other) const noexcept
            {
                return this->m_index < other.m_index;
            }

            /**
             *  Less than or equal operator
             *  @param other Other iterator instance to compare with
             *  @return true if the current iterator is less than or equal to the other iterator, false otherwise
             */
            [[nodiscard]] bool operator<=(const const_iterator& other) const noexcept
            {
                return this->m_index <= other.m_index;
            }

            /**
             *  Equality operator
             *  @param other Other iterator instance to compare with
             *  @return true if the iterators are equal, false otherwise
             */
            [[nodiscard]] bool operator==(const const_iterator& other) const noexcept
            {
                return this->m_index == other.m_index;
            }

            /**
             *  Bigger than or equal operator
             *  @param other Other iterator instance to compare with
             *  @return true if the current iterator is bigger than or equal to the other iterator, false otherwise
             */
            [[nodiscard]] bool operator>=(const const_iterator& other) const noexcept
            {
                return this->m_index >= other.m_index;
            }

            /**
             *  Bigger than operator
             *  @param other Other iterator instance to compare with
             *  @return true if the current iterator is bigger than the other iterator, false otherwise
             */
            [[nodiscard]] bool operator>(const const_iterator& other) const noexcept
            {
                return this->m_index > other.m_index;
            }
        };

        /**
         * Reverse iterator class
         */
        class reverse_iterator : public iterator_base<dynamic_bitset&>
        {
        public:
            using iterator_base<dynamic_bitset&>::iterator_base;

            /**
             * Conversion constructor to const_reverse_iterator
             * @param other Other const_reverse_iterator instance to convert from
             */
            reverse_iterator(const const_reverse_iterator& other) noexcept : iterator_base<dynamic_bitset&>(other.m_bitset, other.m_index) {}

            /**
             * Conversion assign operator to const_reverse_iterator
             * @param other Other const_reverse_iterator instance to convert from
             */
            reverse_iterator& operator=(const const_reverse_iterator& other) noexcept
            {
                this->m_bitset = other.m_bitset;
                this->m_index = other.m_index;
                return *this;
            }

            /**
             * Dereference operator (returns the value of the bit at the current bit index)
             * @return Reference to the bit at the current index
             */
            [[nodiscard]] reference operator*() noexcept
            {
                return reference(this->m_bitset, this->m_index);
            }


            /**
             * Increments the reverse_iterator to the next bit
             */
            reverse_iterator& operator++() noexcept
            {
                --this->m_index;
                return *this;
            }

            /**
             * Increments the reverse_iterator to the next bit, post-increment
             */
            reverse_iterator& operator++(int) noexcept
            {
                reverse_iterator tmp = *this;
                --this->m_index;
                return tmp;
            }

            /**
             * Decrements the reverse_iterator to the previous bit
             */
            reverse_iterator& operator--() noexcept
            {
                ++this->m_index;
                return *this;
            }



            /**
             * Decrements the reverse_iterator to the previous bit, post-decrement
             */
            reverse_iterator& operator--(int) noexcept
            {
                reverse_iterator tmp = *this;
                ++this->m_index;
                return tmp;
            }

            /**
             * Returns the reverse_iterator with the specified amount added to it
             * @param amount Amount to add to the reverse_iterator (bit count)
             * @return New reverse_iterator instance with the specified amount added to it
             */
            [[nodiscard]] reverse_iterator&& operator+(const size_t& amount) const noexcept
            {
                return reverse_iterator(this->m_bitset, this->m_index - amount);
            }
            /**
             * Returns the reverse_iterator with the specified amount subtracted from it
             * @param amount Amount to subtract from the reverse_iterator (bit count)
             * @return New reverse_iterator instance with the specified amount subtracted from it
             */
            [[nodiscard]] reverse_iterator&& operator-(const size_t& amount) const noexcept
            {
                return reverse_iterator(this->m_bitset, this->m_index + amount);
            }

            /**
             * Returns difference between two iterators
             * @param other Other iterator instance subtract with
             * @return Difference between two iterators
             */
            [[nodiscard]] difference_type operator-(const reverse_iterator& other) const noexcept
            {
                return this->m_index + other.m_index;
            }

            /**
             * Returns difference between two iterators
             * @param other Other iterator instance subtract with
             * @return Difference between two iterators
             */
            [[nodiscard]] difference_type operator-(const const_reverse_iterator& other) const noexcept
            {
                return this->m_index + other.m_index;
            }

            /**
             * Adds the specified amount to the reverse_iterator
             * @param amount Amount to add to the reverse_iterator (bit count)
             */
            reverse_iterator& operator+=(const size_t& amount) const noexcept
            {
                this->m_index -= amount;
                return *this;
            }

            /**
             * Subtracts the specified amount to the reverse_iterator
             * @param amount Amount to add to the reverse_iterator (bit count)
             */
            reverse_iterator& operator-=(const size_t& amount) const noexcept
            {
                this->m_index += amount;
                return *this;
            }

            /**
             * Inequality operator
             * @param other Other reverse_iterator instance to compare against
             * @return True if the instances are not equal, false otherwise
             */
            [[nodiscard]] bool operator!=(const reverse_iterator& other) const noexcept
            {
                return this->m_index != other.m_index;
            }

            /**
             *  Less than operator
             *  @param other Other reverse_iterator instance to compare against
             *  @return True if the current instance is less than the other instance, false otherwise
             */
            [[nodiscard]] bool operator<(const reverse_iterator& other) const noexcept
            {
                return this->m_index > other.m_index;
            }

            /**
             *  Less than or equal operator
             *  @param other Other reverse_iterator instance to compare against
             *  @return True if the current instance is less than or equal to the other instance, false otherwise
             */
            [[nodiscard]] bool operator<=(const reverse_iterator& other) const noexcept
            {
                return this->m_index >= other.m_index;
            }

            /**
             *  Equality operator
             *  @param other Other reverse_iterator instance to compare against
             *  @return True if the instances are equal, false otherwise
             */
            [[nodiscard]] bool operator==(const reverse_iterator& other) const noexcept
            {
                return this->m_index == other.m_index;
            }

            /**
             *  Bigger than or equal operator
             *  @param other Other reverse_iterator instance to compare against
             *  @return True if the current instance is bigger than or equal to the other instance, false otherwise
             */
            [[nodiscard]] bool operator>=(const reverse_iterator& other) const noexcept
            {
                return this->m_index >= other.m_index;
            }

            /**
             *  Bigger than operator
             *  @param other Other reverse_iterator instance to compare against
             *  @return True if the current instance is bigger than the other instance, false otherwise
             */
            [[nodiscard]] bool operator>(const reverse_iterator& other) const noexcept
            {
                return this->m_index < other.m_index;
            }
        };

        /**
         * Reverse iterator class
         */
        class const_reverse_iterator : public iterator_base<const dynamic_bitset&>
        {
        public:
            using iterator_base<const dynamic_bitset&>::iterator_base;

            /**
             * Dereference operator (returns the value of the bit at the current bit index)
             * @return Value of the bit at the current index (bit value)
             */
            [[nodiscard]] const_reference operator*() const noexcept
            {
                return this->m_bitset.test(this->m_index);
            }


            /**
             * Increments the reverse_iterator to the next bit
             */
            const_reverse_iterator& operator++() noexcept
            {
                --this->m_index;
                return *this;
            }

            /**
             * Increments the reverse_iterator to the next bit, post-increment
             */
            const_reverse_iterator& operator++(int) noexcept
            {
                const_reverse_iterator tmp = *this;
                --this->m_index;
                return tmp;
            }

            /**
             * Decrements the reverse_iterator to the previous bit
             */
            const_reverse_iterator& operator--() noexcept
            {
                ++this->m_index;
                return *this;
            }

            /**
             * Decrements the reverse_iterator to the previous bit, post-decrement
             */
            const_reverse_iterator& operator--(int) noexcept
            {
                const_reverse_iterator tmp = *this;
                ++this->m_index;
                return tmp;
            }

            /**
             * Returns the reverse_iterator with the specified amount added to it
             * @param amount Amount to add to the reverse_iterator (bit count)
             * @return New reverse_iterator instance with the specified amount added to it
             */
            [[nodiscard]] const_reverse_iterator&& operator+(const size_t& amount) const noexcept
            {
                return const_reverse_iterator(this->m_index, this->m_index - amount);
            }
            /**
             * Returns the reverse_iterator with the specified amount subtracted from it
             * @param amount Amount to subtract from the reverse_iterator (bit count)
             * @return New reverse_iterator instance with the specified amount subtracted from it
             */
            [[nodiscard]] const_reverse_iterator&& operator-(const size_t& amount) const noexcept
            {
                return const_reverse_iterator(this->m_index, this->m_index + amount);
            }

            /**
             * Returns difference between two iterators
             * @param other Other iterator instance subtract with
             * @return Difference between two iterators
             */
            [[nodiscard]] difference_type operator-(const const_reverse_iterator& other) const noexcept
            {
                return this->m_index - other.m_index;
            }

            /**
             * Returns difference between two iterators
             * @param other Other iterator instance subtract with
             * @return Difference between two iterators
             */
            [[nodiscard]] difference_type operator-(const reverse_iterator& other) const noexcept
            {
                return this->m_index - other.m_index;
            }

            /**
             * Adds the specified amount to the reverse_iterator
             * @param amount Amount to add to the reverse_iterator (bit count)
             */
            const_reverse_iterator& operator+=(const size_t& amount) noexcept
            {
                this->m_index -= amount;
                return *this;
            }

            /**
             * Subtracts the specified amount to the reverse_iterator
             * @param amount Amount to add to the reverse_iterator (bit count)
             */
            const_reverse_iterator& operator-=(const size_t& amount) noexcept
            {
                this->m_index += amount;
                return *this;
            }

            /**
             * Inequality operator
             * @param other Other const_reverse_iterator instance to compare against
             * @return true if instances are not equal, false otherwise
             */
            [[nodiscard]] bool operator!=(const const_reverse_iterator& other) const noexcept
            {
                return this->m_index != other.m_index;
            }

            /**
             *  Less than operator
             *  @param other Other const_reverse_iterator instance to compare against
             *  @return true if the current instance is less than the other instance, false otherwise
             */
            [[nodiscard]] bool operator<(const const_reverse_iterator& other) const noexcept
            {
                return this->m_index > other.m_index;
            }

            /**
             *  Less than or equal operator
             *  @param other Other const_reverse_iterator instance to compare against
             *  @return true if the current instance is less than or equal to the other instance, false otherwise
             */
            [[nodiscard]] bool operator<=(const const_reverse_iterator& other) const noexcept
            {
                return this->m_index >= other.m_index;
            }

            /**
             *  Equality operator
             *  @param other Other const_reverse_iterator instance to compare against
             *  @return true if instances are equal, false otherwise
             */
            [[nodiscard]] bool operator==(const const_reverse_iterator& other) const noexcept
            {
                return this->m_index == other.m_index;
            }

            /**
             *  Bigger than or equal operator
             *  @param other Other const_reverse_iterator instance to compare against
             *  @return true if the current instance is bigger than or equal to the other instance, false otherwise
             */
            [[nodiscard]] bool operator>=(const const_reverse_iterator& other) const noexcept
            {
                return this->m_index >= other.m_index;
            }

            /**
             *  Bigger than operator
             *  @param other Other const_reverse_iterator instance to compare against
             *  @return true if the current instance is bigger than the other instance, false otherwise
             */
            [[nodiscard]] bool operator>(const const_reverse_iterator& other) const noexcept
            {
                return this->m_index < other.m_index;
            }
        };

        /**
         * Empty constructor
         */
        dynamic_bitset() noexcept : m_partial_size(0), m_storage_size(0), m_size(0), m_data(nullptr) {}

        /**
		 * Size constructor
		 * @param size Size of the bitset to create (bit count)
		 */
        explicit dynamic_bitset(const size_t& size) noexcept : m_partial_size(size % m_block_size), m_storage_size(size / m_block_size + !!m_partial_size), m_size(size), m_data(new BlockType[m_storage_size])
        {
            clear();
        }

        /**
         * Size and bool value constructor
         * @tparam U Used to constrain type to bool. [don't use]
         * @param size Size of the bitset to create (bit count)
         * @param val Bool value to fill the bitset with (bit value)
         */
        template <unsigned_integer U = std::remove_cvref_t<BlockType>> requires (!std::is_same_v<bool, U> && !std::is_pointer_v<U> && !std::is_array_v<U>)
        explicit dynamic_bitset(const size_t& size, const bool& val) noexcept : m_partial_size(size % m_block_size), m_storage_size(size / m_block_size + !!m_partial_size), m_size(size), m_data(new BlockType[m_storage_size])
        {
	        fill(val);
		}

        /**
         * Size and block value constructor
         * @param size Size of the bitset to create (bit count)
         * @param block Block to fill the bitset with (block value)
         */
        explicit dynamic_bitset(const size_t& size, const BlockType& block) noexcept : m_partial_size(size % m_block_size), m_storage_size(size / m_block_size + !!m_partial_size), m_size(size), m_data(new BlockType[m_storage_size])
        {
            fill_block(block);
        }

        /**
         * Copy constructor
         * @param other Other dynamic_bitset instance to copy from
         */
        dynamic_bitset(const dynamic_bitset& other) noexcept : m_partial_size(other.m_partial_size), m_storage_size(other.m_storage_size), m_size(other.m_size), m_data(new BlockType[other.m_storage_size])
        {
            //std::copy(other.m_data, other.m_data + other.m_storage_size, m_data);
            _from_other(other);
        }

        /**
         * Size copy constructor
         * @param size Size of the bitset to create (bit count)
		 * @param other Other dynamic_bitset instance to copy from
         */
    	dynamic_bitset(const size_t& size, const dynamic_bitset& other) noexcept : m_partial_size(size % m_block_size), m_storage_size(size / m_block_size + !!m_partial_size), m_size(size), m_data(new BlockType[m_storage_size])
        {
            //::memset(m_data, 0, m_storage_size * sizeof(BlockType));;
            //std::copy(other.m_data, other.m_data + (std::min)(m_storage_size, other.m_storage_size), m_data);
            _from_other(other);
        }

        /**
         * Move constructor
         * @param other Other dynamic_bitset instance to move from
         */
        dynamic_bitset(dynamic_bitset&& other) noexcept : m_partial_size(other.m_partial_size), m_storage_size(other.m_storage_size), m_size(other.m_size), m_data(other.m_data)
        {
            //other.m_partial_size = other.m_storage_size = other.m_size = other.m_data = 0;
            _from_other(other);
        }

        /**
         * General conversion constructor
         * @tparam OtherBlockType Type of the block to copy from
         * @param other Other bitset instance to copy from
         */
        template <unsigned_integer OtherBlockType> requires (std::convertible_to<OtherBlockType, BlockType> && !std::is_same_v<BlockType, OtherBlockType>)
    	dynamic_bitset(const dynamic_bitset<OtherBlockType>& other) noexcept : m_partial_size(other.m_size % m_block_size), m_storage_size(static_cast<size_t>(static_cast<double>(sizeof(OtherBlockType) / sizeof(BlockType)) * other.m_storage_size + !!m_partial_size)), m_size(other.m_size), m_data(new BlockType[m_storage_size])
        {
            clear();
            _from_other<OtherBlockType>(other);
        }

        /**
		 * Size and general conversion constructor
		 * @tparam OtherBlockType Type of the block to copy from
		 * @param size Size of bitset to create (bit count)
		 * @param other Other bitset instance to copy from
		 */
        template <unsigned_integer OtherBlockType> requires (std::convertible_to<OtherBlockType, BlockType> && !std::is_same_v<BlockType, OtherBlockType>)
        explicit dynamic_bitset(const size_t& size, const dynamic_bitset<OtherBlockType>& other) noexcept : m_partial_size(size % m_block_size), m_storage_size(size / sizeof(BlockType) + !!m_partial_size), m_size(size), m_data(new BlockType[m_storage_size])
        {
            clear();
            _from_other<OtherBlockType>(other);
        }

        /**
         * C string stack array conversion constructor
         * @tparam Elem Type of the character array
         * @tparam StrSize m_size of the character array
         * @param c_str Array c string to fill the bitset with (must be null-terminated)
         * @param set_chr Character that represents set bits
         */
        template <char_type Elem, size_t StrSize>
        dynamic_bitset(const Elem(&c_str)[StrSize], const Elem& set_chr = '1') noexcept : m_partial_size((StrSize - 1) % m_block_size), m_storage_size((StrSize - 1) / m_block_size + !!m_partial_size), m_size(StrSize - 1), m_data(new BlockType[m_storage_size])
        {
            clear();
            _from_c_string<Elem, StrSize>(c_str, set_chr);
        }

        /**
		 * Size and c string array conversion constructor
		 * @tparam Elem Type of the character array
		 * @tparam StrSize m_size of the character array
		 * @param size Size of the bitset to create (bit count)
		 * @param c_str Array c string to fill the bitset with (must be null-terminated)
		 * @param set_chr Character that represents set bits
		 */
        template <char_type Elem, size_t StrSize>
        explicit dynamic_bitset(const size_t& size, const Elem(&c_str)[StrSize], const Elem& set_chr = '1') noexcept : m_partial_size(size % m_block_size), m_storage_size(size / m_block_size + !!m_partial_size), m_size(size), m_data(new BlockType[m_storage_size])
        {
            clear();
            _from_c_string<Elem, StrSize>(c_str, set_chr);
        }

        /**
         * Size and c string pointer conversion constructor
         * @tparam PtrArr Type of the character array [don't use]
         * @tparam Elem Type of the character array [don't use]
         * @param size Size of bitset to create (bit count)
         * @param c_str Pointer to c string to fill the bitset with (must be null-terminated)
         * @param set_chr Character that represents set bits
         */
        template <typename PtrArr = const char*, char_type Elem = std::remove_cvref_t<std::remove_pointer_t<std::remove_cvref_t<PtrArr>>>> requires (std::is_pointer_v<PtrArr> && !std::is_array_v<PtrArr>&& char_type<Elem>)
        explicit dynamic_bitset(const size_t& size, PtrArr c_str, const Elem& set_chr = '1') noexcept : m_partial_size(size % m_block_size), m_storage_size(size / m_block_size + !!m_partial_size), m_size(size), m_data(new BlockType[m_storage_size])
        {
            clear();
            _from_c_string<PtrArr, Elem>(c_str, set_chr);
        }

        /**
         * String conversion constructor
         * @tparam Elem Type of the character array
         * @param str String to fill the bitset with (must be null-terminated), it's size ought to be >= size(), but nothing will happen if it's not
         * @param set_chr Character that represents set bits
         */
        template <char_type Elem>
        dynamic_bitset(const std::basic_string<Elem>& str, const Elem& set_chr = '1') noexcept : m_partial_size(str.size() % m_block_size), m_storage_size(str.size() / m_block_size + !!m_partial_size), m_size(str.size()), m_data(new BlockType[m_storage_size])
        {
            clear();
            _from_string(str, set_chr);
        }

        /**
		 * Size and string conversion constructor
		 * @tparam Elem Type of the character array
		 * @param size Size of bitset to create (bit count)
		 * @param str String to fill the bitset with (must be null-terminated), it's size ought to be >= size(), but nothing will happen if it's not
		 * @param set_chr Character that represents set bits
		 * @param sep_present Whenever character that separates the bit blocks exists
		 */
        template <typename Elem>
        explicit dynamic_bitset(const size_t& size, const std::basic_string<Elem>& str, const Elem& set_chr = '1') noexcept : m_partial_size(size % m_block_size), m_storage_size(size / m_block_size + !!m_partial_size), m_size(size), m_data(new BlockType[m_storage_size])
        {
            clear();
            _from_string(str, set_chr);
        }

        /**
         * Bool array conversion constructor
         * @tparam ArrType Used to constrain type. [don't use]
         * @tparam OtherSize Size of the bool array to copy from
         * @param arr Bool array containing the values to initialize the bitset with (bit values).
         */
        template <typename ArrType = bool, uint64_t OtherSize> requires (std::is_same_v<ArrType, bool>)
        dynamic_bitset(const bool (&arr)[OtherSize]) noexcept : m_partial_size(OtherSize % m_block_size), m_storage_size(OtherSize / m_block_size + !!m_partial_size), m_size(OtherSize), m_data(new BlockType[m_storage_size])
        {
            clear();
            _from_bool_array<OtherSize>(arr);
        }

        /**
		 * Size and bool array conversion constructor
         * @tparam ArrType Used to constrain type. [don't use]
		 * @tparam OtherSize Size of the bool array to copy from
		 * @param size Size of the bitset to create (bit count)
		 * @param arr Bool array containing the values to initialize the bitset with (bit values).
		 */
        template <typename ArrType = bool, uint64_t OtherSize> requires (std::is_same_v<ArrType, bool>)
        explicit dynamic_bitset(const size_t& size, const ArrType(&arr)[OtherSize]) noexcept : m_partial_size(size % m_block_size), m_storage_size(size / m_block_size + !!m_partial_size), m_size(size), m_data(new BlockType[m_storage_size])
        {
            clear();
            _from_bool_array<OtherSize>(arr);
        }

        /**
		 * Size and bool pointer array conversion constructor
		 * @tparam PtrArr Used to constrain type [don't use]
		 * @param size Size of the bitset to create (bit count)
		 * @param ptr Pointer to the bool array containing the values to convert from (bit values). It's size must be >= size
		 */
        template <typename PtrArr = const bool*> requires (std::is_pointer_v<PtrArr> && !std::is_array_v<PtrArr>&& std::is_same_v<const bool*, PtrArr> && !std::is_same_v<std::remove_cvref_t<BlockType>, bool*>)
        explicit dynamic_bitset(const size_t& size, PtrArr ptr) noexcept : m_partial_size(size % m_block_size), m_storage_size(size / m_block_size + !!m_partial_size), m_size(size), m_data(new BlockType[m_storage_size])
        {
            clear();
            _from_bool_array<PtrArr>(ptr);
        }

        /**
         * Block stack array copy constructor
         * @tparam OtherSize m_size of the other bitset to copy from
         * @param arr Array of BlockType values to initialize the bitset with (block values).
         */
        template <size_t OtherSize>
        dynamic_bitset(const BlockType(&arr)[OtherSize]) noexcept : m_partial_size(0), m_storage_size(OtherSize), m_size(m_storage_size * m_block_size), m_data(new BlockType[m_storage_size])
        {
            _from_block_array<OtherSize>(arr);
        }

        /**
		 * Size and block stack array copy constructor
		 * @tparam OtherSize size of the block array
		 * @param size Size of the bitset to create (bit count)
		 * @param arr Array of BlockType values to initialize the bitset with (block values).
		 */
        template <size_t OtherSize>
        explicit dynamic_bitset(const size_t& size, const BlockType(&arr)[OtherSize]) noexcept : m_partial_size(size % m_block_size), m_storage_size(size / m_block_size + !!m_partial_size), m_size(size), m_data(new BlockType[m_storage_size])
        {
            _from_block_array<OtherSize>(arr);
        }

        /**
         * Size and block pointer array copy constructor
         * @tparam PtrArr Type of the pointer array [don't use]
         * @param size Size of bitset to create (bit count)
         * @param ptr Pointer to array of BlockType values to initialize the bitset with (block values). It's size must be >= storage_size()
         */
        template <typename PtrArr = const BlockType*> requires (std::is_pointer_v<PtrArr> && !std::is_array_v<PtrArr>&& std::is_same_v<BlockType, std::remove_cvref_t<std::remove_pointer_t<PtrArr>>>)
        explicit dynamic_bitset(const size_t& size, const PtrArr ptr) noexcept : m_partial_size(size % m_block_size), m_storage_size(size / m_block_size + !!m_partial_size), m_size(size), m_data(new BlockType[m_storage_size])
        {
            _from_block_array<PtrArr>(ptr);
        }

        /**
         * Stack block array copy constructor with different block type
         * @tparam OtherBlockType Type of the block to copy from
         * @tparam OtherSize m_size of the other bitset to copy from
         * @param arr Array of block values to initialize bitset with.
         */
        template <unsigned_integer OtherBlockType, size_t OtherSize> requires (std::convertible_to<OtherBlockType, BlockType> && !std::is_same_v<BlockType, OtherBlockType>)
        dynamic_bitset(const OtherBlockType(&arr)[OtherSize]) noexcept : m_partial_size(0), m_storage_size(static_cast<size_t>(std::ceil(static_cast<double>(sizeof(OtherBlockType)) / sizeof(BlockType) * OtherSize))), m_size(m_storage_size * m_block_size), m_data(new BlockType[m_storage_size])
        {
            clear();
            _from_block_array<OtherBlockType, OtherSize>(arr);
        }

		/**
		 * Size and stack block array conversion constructor with different block type
		 * @tparam OtherBlockType Type of the block to copy from
		 * @tparam OtherSize m_size of the other bitset to copy from
		 * @param size Size of the bitset to create (bit count)
		 * @param arr Array of block values to initialize bitset with.
		 */
        template <unsigned_integer OtherBlockType, size_t OtherSize> requires (std::convertible_to<OtherBlockType, BlockType> && !std::is_same_v<BlockType, OtherBlockType>)
    	explicit dynamic_bitset(const size_t& size, const OtherBlockType(&arr)[OtherSize]) noexcept : m_partial_size(size % m_block_size), m_storage_size(size / m_block_size + !!m_partial_size), m_size(size), m_data(new BlockType[m_storage_size])
        {
            clear();
            _from_block_array<OtherBlockType, OtherSize>(arr);
        }

        /**
         * Size and pointer array conversion constructor with different block type
         * @tparam PtrArr Type of the block pointer array [don't use]
         * @tparam Elem Type of elem in the block pointer array [don't use]
         * @param size Size of bitset to create (bit count)
         * @param ptr Pointer to array of block values to initialize the bitset with. It's size must be >= storage_size()
         */
        template <typename PtrArr, unsigned_integer Elem = std::remove_cvref_t<std::remove_pointer_t<std::remove_cvref_t<PtrArr>>>> requires (std::convertible_to<Elem, BlockType>&& std::convertible_to<BlockType, Elem> && !std::is_same_v<BlockType, Elem>&& std::is_pointer_v<PtrArr> && !std::is_array_v<PtrArr> && !std::is_same_v<bool, Elem>)
        explicit dynamic_bitset(const size_t& size, PtrArr const& ptr) noexcept : m_partial_size(size % m_block_size), m_storage_size(size / m_block_size + !!m_partial_size), m_size(size), m_data(new BlockType[m_storage_size])
        {
            clear();
            _from_block_array<PtrArr, Elem>(ptr);
        }

        /**
		 * Destructor
		 */
        ~dynamic_bitset() noexcept
        {
            delete[] m_data;
        }

        /**
         * Returns the value of the bit at the specified index
         * @param index Index of the bit to retrieve (bit index)
         * @return Value of the bit at the specified index (bit value)
         */
        [[nodiscard]] const_reference operator[](const size_t& index) const noexcept
        {
            return m_data[index / m_block_size] & BlockType{ 1 } << index % m_block_size;
        }

        /**
         * Returns reference to the value of the bit at the specified index
         * @param index Index of the bit to retrieve (bit index)
         * @return Reference to the value of the bit at the specified index
         */
        [[nodiscard]] reference operator[](const size_t& index) noexcept
        {
            return reference(*this, index);
        }

        /**
         * General copy assignment operator
         * @tparam OtherBlockType Type of the block to copy from
         * @param other Other dynamic_bitset instance to copy from
         */
        template <unsigned_integer OtherBlockType> requires (std::convertible_to<OtherBlockType, BlockType> && !std::is_same_v<BlockType, OtherBlockType>)
        dynamic_bitset& operator=(const dynamic_bitset<OtherBlockType>& other) noexcept
        {
            ::memset(m_data, 0, m_storage_size * sizeof(BlockType));

            if (sizeof(BlockType) > sizeof(OtherBlockType))
            {
                constexpr uint16_t diff = sizeof(BlockType) / sizeof(OtherBlockType);

                for (size_t i = 0; i < m_storage_size; ++i)
                {
                    for (uint16_t j = 0; j < diff; ++j)
                    {
                        if (i * diff + j >= other.m_storage_size)
                            return *this;
	                    m_data[i] |= static_cast<BlockType>(other.m_data[i * diff + j]) << j * other.m_block_size;
                    }
                }
            }
            else
            {
                constexpr uint16_t diff = sizeof(OtherBlockType) / sizeof(BlockType);

                for (size_t i = 0; i < other.m_storage_size; ++i)
                {
                    for (uint16_t j = 0; j < diff; ++j)
                    {
                        if (i * diff + j >= m_storage_size)
                            return *this;
	                    m_data[i * diff + j] = other.m_data[i] >> j % diff * m_block_size;
                    }
                }
            }

            return *this;
        }

        /**
         * Copy assignment operator
         * @param other Other dynamic_bitset instance to copy from
         */
        dynamic_bitset& operator=(const dynamic_bitset& other) noexcept
        {
            ::memset(m_data, 0, m_storage_size * sizeof(BlockType));

            std::copy(other.m_data, other.m_data + (std::min)(m_storage_size, other.m_storage_size), m_data);
            return *this;
        }

        /**
		 * Move assignment operator
		 * @param other Other dynamic_bitset instance to move from
		 */
        dynamic_bitset& operator=(dynamic_bitset&& other) noexcept
        {
            m_partial_size = other.m_partial_size;
            m_storage_size = other.m_storage_size;
            m_size = other.m_size;
            m_data = other.m_data;

            other.m_partial_size = other.m_storage_size = other.m_size = 0;
            other.m_data = nullptr;

        	return *this;
        }

        // TODO: General comp & bitwise operators

        // comparison operators

        /**
         * Equality operator
         * @param other Other bitset instance to compare with
         * @return True if the two instances are equal, false otherwise
         */
        [[nodiscard]] bool operator==(const dynamic_bitset& other) const noexcept
        {
            if (m_size != other.m_size)
                return false;
            for (size_t i = 0; i < m_storage_size - !!m_partial_size; ++i)
            {
                if (m_data[i] != other.m_data[i])
                    return false;
            }
            if (m_partial_size)
            {
                const BlockType mask = (1 << m_partial_size) - 1;
                if ((m_data[m_storage_size - 1] & mask) != (other.m_data[m_storage_size - 1] & mask))
                    return false;
            }
            return true;
        }

        /**
         * Inequality operator
         * @param other Other bitset instance to compare with
         * @return True if the two instances are not equal, false otherwise
         */
        [[nodiscard]] bool operator!=(const dynamic_bitset& other) const noexcept
        {
            if (m_size != other.m_size)
            for (size_t i = 0; i < m_storage_size - !!m_partial_size; ++i)
            {
                if (m_data[i] == other.m_data[i])
                    return false;
            }
            if (m_partial_size)
            {
                const BlockType mask = (1 << m_partial_size) - 1;
                if ((m_data[m_storage_size - 1] & mask) == (other.m_data[m_storage_size - 1] & mask))
                    return false;
            }
            return true;
        }

        // Bitwise operators

        /**
         * Bitwise AND operator
         * @param other Other bitset instance to perform the operation with
         * @return New bitset instance containing the result of the operation
         */
        [[nodiscard]] dynamic_bitset operator&(const dynamic_bitset& other) const noexcept
        {
            dynamic_bitset result;
            for (size_t i = 0; i < result.m_storage_size; ++i)
                result.m_data[i] = m_data[i] & other.m_data[i];
            return result;
        }

        /**
         * Apply bitwise AND operation with another bitset instance
         * @param other Other bitset instance to perform the operation with
         */
        dynamic_bitset& operator&=(const dynamic_bitset& other) noexcept
        {
            for (size_t i = 0; i < m_storage_size; ++i)
                m_data[i] &= other.m_data[i];
            return *this;
        }

        /**
         * Bitwise OR operator
         * @param other Other bitset instance to perform the operation with
         * @return New bitset instance containing the result of the operation
         */
        [[nodiscard]] dynamic_bitset operator|(const dynamic_bitset& other) const noexcept
        {
            dynamic_bitset result;
            for (size_t i = 0; i < result.m_storage_size; ++i)
                result.m_data[i] = m_data[i] | other.m_data[i];
            return result;
        }

        /**
         * Apply bitwise OR operation with another bitset instance
         * @param other Other bitset instance to perform the operation with
         */
        dynamic_bitset& operator|=(const dynamic_bitset& other) noexcept
        {
            for (size_t i = 0; i < m_storage_size; ++i)
                *m_data[i] |= other.m_data[i];
            return *this;
        }

        /**
         * Bitwise XOR operator
         * @param other Other bitset instance to perform the operation with
         * @return New bitset instance containing the result of the operation
         */
        [[nodiscard]] dynamic_bitset operator^(const dynamic_bitset& other) const noexcept
        {
            dynamic_bitset result;
            for (size_t i = 0; i < result.m_storage_size; ++i)
                result.m_data[i] = m_data[i] ^ other.m_data[i];
            return result;
        }

        /**
         * Apply bitwise XOR operation with another bitset instance
         * @param other Other bitset instance to perform the operation with
         */
        dynamic_bitset& operator^=(const dynamic_bitset& other) noexcept
        {
            for (size_t i = 0; i < m_storage_size; ++i)
                m_data[i] ^= other.m_data[i];
            return *this;
        }

        /**
         * Bitwise NOT operator
         * @return New bitset instance containing the result of the operation
         */
        [[nodiscard]] dynamic_bitset operator~() const noexcept
        {
            dynamic_bitset result;
            for (size_t i = 0; i < m_storage_size; ++i)
                result.m_data[i] = ~m_data[i];
            return result;
        }

        /**
         * Bitwise right shift operator
         * @param shift Amount of bits to shift to the right
         * @return New bitset instance containing the result of the operation
         */
        [[nodiscard]] dynamic_bitset operator>>(const size_t& shift) const noexcept
        {
            dynamic_bitset result;
            if (shift <= m_block_size)
            {
                for (size_t i = 0; i < m_storage_size; ++i)
                    result.m_data[i] = m_data[i] >> shift;
            }
            return result;
        }

        /**
         * Apply bitwise right shift operation
         * @param shift Amount of bits to shift to the right
         */
        dynamic_bitset& operator>>=(const size_t& shift) noexcept
        {
            if (shift > m_block_size)
                clear();
            else
            {
                for (size_t i = 0; i < m_storage_size; ++i)
                    m_data[i] >>= shift;
            }
            return *this;
        }


        /**
         * Bitwise right shift operator
         * @param shift Amount of bits to shift to the right
         * @return New bitset instance containing the result of the operation
         */
        [[nodiscard]] dynamic_bitset operator<<(const size_t& shift) const noexcept
        {
            dynamic_bitset result;
            if (shift <= m_block_size)
            {
                for (size_t i = 0; i < m_storage_size; ++i)
                    result.m_data[i] = m_data[i] << shift;
            }
            return result;
        }

        /**
         * Apply bitwise right shift operation
         * @param shift Amount of bits to shift to the right
         */
        dynamic_bitset& operator<<=(const size_t& shift) noexcept
        {
            if (shift > m_block_size)
                clear();
            else
            {
                for (size_t i = 0; i < m_storage_size; ++i)
                    m_data[i] <<= shift;
            }
            return *this;
        }

        /**
         * Difference operator
         * @param other Other bitset instance to compare with
         * @return New bitset instance containing the result of the operation
         */
        [[nodiscard]] dynamic_bitset operator-(const dynamic_bitset& other) const noexcept
        {
            dynamic_bitset result;
            for (size_t i = 0; i < result.m_storage_size; ++i)
                result.m_data[i] = m_data[i] & ~other.m_data[i];
            return result;
        }

        /**
         * Apply difference operation
         * @param other Other bitset instance to compare with
         */
        dynamic_bitset& operator-=(const dynamic_bitset& other) noexcept
        {
            for (size_t i = 0; i < m_storage_size; ++i)
                m_data[i] &= ~other.m_data[i];
            return *this;
        }

        // Conversion functions/operators

    private:

        /**
         * Copy function
         * @param other Other dynamic_bitset instance to copy from
         */
        void _from_other(const dynamic_bitset& other) noexcept
        {
            if (this != &other)
                std::copy(other.m_data, other.m_data + (std::min<size_t>)(m_storage_size, other.m_storage_size), m_data);
            for (size_t i = other.m_storage_size; i < m_storage_size; ++i)
                *(m_data + i) = 0;
        }

        /**
         * Move function
         * @param other Other dynamic_bitset instance to move from
         */
        static void _from_other(dynamic_bitset&& other) noexcept
        {
            other.m_partial_size = other.m_partial_size = other.m_size = 0;
            other.m_data = nullptr;
        }

        /**
         * Conversion function with different block type and size
         * @tparam OtherBlockType Type of the block to convert from
         * @tparam OtherSize Size of the other bitset to convert from
         * @param other Other bitset instance to convert from
         */
        template <unsigned_integer OtherBlockType> requires (std::convertible_to<OtherBlockType, BlockType> && !std::is_same_v<BlockType, OtherBlockType>)
        void _from_other(const dynamic_bitset<OtherBlockType>& other) noexcept
        {
            clear();
            if (sizeof(BlockType) > sizeof(OtherBlockType))
            {
                constexpr uint16_t diff = sizeof(BlockType) / sizeof(OtherBlockType);

                for (size_t i = 0; i < m_storage_size; ++i)
                {
                    for (size_t j = 0; j < diff; ++j)
                    {
                        if (i * diff + j >= other.m_storage_size)
                        {
                            return;
                        }
                        *(m_data + i) |= static_cast<BlockType>(other.m_data[i * diff + j]) << j * other.m_block_size;
                    }
                }
            }
            else
            {
                constexpr uint16_t diff = sizeof(OtherBlockType) / sizeof(BlockType);

                for (size_t i = 0; i < other.m_storage_size; ++i)
                {
                    for (uint16_t j = 0; j < diff; ++j)
                    {
                        if (i * diff + j >= m_storage_size)
                        {
                            return;
                        }
                        *(m_data + i * diff + j) = other.m_data[i] >> j % diff * m_block_size;
                    }
                }
            }
        }

        /**
		 * String to bitset conversion function
		 * @tparam Elem Type of the character array
		 * @param str String to convert from (must be null-terminated)
		 * @param set_chr Character that represents set bits
		 */
        template <char_type Elem>
        void _from_string(const std::basic_string<Elem>& str, const Elem& set_chr = '1') noexcept
        {
            for (size_t i = 0; i < m_storage_size; ++i)
            {
                for (uint16_t j = 0; j < m_block_size; ++j)
                {
                    if (i * m_block_size + j >= str.size())
                    {
                        return;
                    }
                    m_data[i] |= str[i * m_block_size + j] == set_chr ? BlockType{ 1 } << j : 0;
                }
            }
        }

        /**
         * Block pointer array copy function
         * @tparam PtrArr Type of the pointer array [don't use]
         * @param ptr Pointer to array of BlockType values to initialize the bitset with (block values). It's size must be >= storage_size()
         */
        template <typename PtrArr = const BlockType*> requires (std::is_pointer_v<PtrArr> && !std::is_array_v<PtrArr>&& std::is_same_v<BlockType, std::remove_cvref_t<std::remove_pointer_t<std::remove_cvref_t<PtrArr>>>> && !std::is_same_v<bool, std::remove_cvref_t<std::remove_pointer_t<std::remove_cvref_t<PtrArr>>>>)
        void _from_block_array(PtrArr ptr) noexcept
        {
            for (size_t i = 0; i < m_storage_size; ++i)
                m_data[i] = *ptr++;
        }

        /**
         * Block array copy function
         * @tparam @tparam OtherSize size of the block array
         * @param arr Array of BlockType values to initialize the bitset with (block values).
         */
        template <size_t OtherSize>
        void _from_block_array(const BlockType(&arr)[OtherSize]) noexcept
        {
            for (size_t i = 0; i < (std::min)(m_storage_size, OtherSize); ++i)
                m_data[i] = arr[i];
            for (size_t i = OtherSize; i < m_storage_size; ++i)
                m_data[i] = 0;
        }

        /**
         * Block stack array conversion constructor with different size and block type
         * @tparam OtherBlockType Type of the block to convert from
         * @tparam OtherSize Size of the other bitset to convert from
         * @param arr Array of OtherBlockType values to convert from.
         */
        template <unsigned_integer OtherBlockType, size_t OtherSize> requires (std::convertible_to<OtherBlockType, BlockType> && !std::is_same_v<BlockType, OtherBlockType>)
        void _from_block_array(const OtherBlockType(&arr)[OtherSize]) noexcept
        {
            if (sizeof(BlockType) > sizeof(OtherBlockType))
            {
                constexpr uint16_t diff = sizeof(BlockType) / sizeof(OtherBlockType), other_block_size = sizeof(OtherBlockType) * 8;

                for (size_t i = 0; i < m_storage_size; ++i)
                {
                    for (size_t j = 0; j < diff; ++j)
                    {
                        if (i * diff + j > OtherSize)
                        {
                            return;
                        }
                        m_data[i] |= static_cast<BlockType>(arr[i * diff + j]) << j * other_block_size;
                    }
                }
            }
            else
            {
                constexpr uint16_t diff = sizeof(OtherBlockType) / sizeof(BlockType);

                for (size_t i = 0; i < OtherSize; ++i)
                {
                    for (uint16_t j = 0; j < diff; ++j)
                    {
                        if (i * diff + j > m_storage_size)
                        {
                            return;
                        }
                        m_data[i * diff + j] = arr[i] >> j % diff * m_block_size;
                    }
                }
            }
        }

        /**
         * Block stack pointer array conversion function with different block type and size
         * @tparam PtrArr Type of the block pointer array [don't use]
         * @tparam Elem Type of elem in the block pointer array [don't use]
         * @param ptr Pointer to array of BlockType values to initialize the bitset with (block values). It's size must be >= storage_size()
         */
        template <typename PtrArr, unsigned_integer Elem = std::remove_cvref_t<std::remove_pointer_t<std::remove_cvref_t<PtrArr>>>> requires (std::convertible_to<Elem, BlockType>&& std::convertible_to<BlockType, Elem> && !std::is_same_v<BlockType, Elem>&& std::is_pointer_v<PtrArr> && !std::is_array_v<PtrArr> && !std::is_same_v<bool, Elem>)
        void _from_block_array(PtrArr const& ptr) noexcept
        {
            if (sizeof(BlockType) > sizeof(Elem))
            {
                constexpr uint16_t diff = sizeof(BlockType) / sizeof(Elem), other_block_size = sizeof(Elem) * 8;

                for (size_t i = 0; i < m_storage_size; ++i)
                {
                    for (size_t j = 0; j < diff; ++j)
                        m_data[i] |= static_cast<BlockType>(*(ptr + i * diff + j)) << j * other_block_size;
                }
            }
            else
            {
                constexpr uint16_t diff = sizeof(Elem) / sizeof(BlockType);

                for (size_t i = 0; i < m_storage_size; ++i)
                {
                    for (uint16_t j = 0; j < diff; ++j)
                        m_data[i * diff + j] = *(ptr + i) >> j % diff * m_block_size;
                }
            }
        }

        /**
         * C string stack array to bitset conversion function
         * @tparam Elem Type of the character array
         * @tparam StrSize Size of the character array
         * @param c_str Array c string to fill the bitset with (must be null-terminated)
         * @param set_chr Character that represents set bits
         */
        template <char_type Elem, size_t StrSize>
        void _from_c_string(const Elem(&c_str)[StrSize], const Elem& set_chr = '1') noexcept
        {
            for (size_t i = 0; i < m_storage_size; ++i)
            {
                for (uint16_t j = 0; j < m_block_size; ++j)
                {
                    if (!c_str[i * m_block_size + j])
                    {
                        return;
                    }
                    m_data[i] |= c_str[i * m_block_size + j] == set_chr ? BlockType{ 1 } << j : 0;
                }
            }
        }

        /**
         * C string pointer array to bitset conversion function
         * @tparam PtrArr Type of the character array [don't use]
         * @tparam Elem Type of the character array [don't use]
         * @param c_str Pointer to c string to fill the bitset with (must be null-terminated)
         * @param set_chr Character that represents set bits
         */
        template <typename PtrArr = const char*, char_type Elem = std::remove_cvref_t<std::remove_pointer_t<std::remove_cvref_t<PtrArr>>>> requires (std::is_pointer_v<PtrArr> && !std::is_array_v<PtrArr>)
        void _from_c_string(PtrArr c_str, const Elem& set_chr = '1') noexcept
        {
            for (size_t i = 0; i < m_storage_size; ++i)
            {
                for (uint16_t j = 0; j < m_block_size; ++j)
                {
                    if (!*c_str)
                    {
                        return;
                    }
                    m_data[i] |= *c_str++ == set_chr ? BlockType{ 1 } << j : 0;
                }
            }
        }

    public:

        /**
         * bitset to C string conversion function
         * @param set_chr Character to represent set bits
         * @param rst_chr Character to represent clear bits
         * @tparam Elem Type of character in the array
         * @return C string representation of the bitset
         */
        template <char_type Elem = char>
        [[nodiscard]] Elem* to_c_string(const Elem& set_chr = '1', const Elem& rst_chr = '0') const noexcept
        {
            Elem* result = new Elem[m_size + 1];
            for (size_t i = 0; i < m_storage_size - !!m_partial_size; ++i)
            {
                for (uint16_t j = 0; j < m_block_size; ++j)
                    *(result + i * m_block_size + j) = m_data[i] & BlockType{ 1 } << j ? set_chr : rst_chr;
            }
            if (m_partial_size)
            {
                for (size_t i = 0; i < m_partial_size; ++i)
                    *(result + (m_storage_size - 1) * m_block_size + i) = m_data[i] & BlockType{ 1 } << i ? set_chr : rst_chr;
            }
            *(result + m_size) = '\0';
            return result;
        }

    private:

        /**
         * Bool array to bitset conversion function
         * @tparam OtherSize Size of the bool array to copy from
         * @param arr Bool array containing the values to convert from (bit values).
         */
        template <uint64_t OtherSize>
        void _from_bool_array(const bool(&arr)[OtherSize]) noexcept
        {
            for (size_t i = 0; i < m_storage_size - !!m_partial_size; ++i)
            {
                for (uint16_t j = 0; j < m_block_size; ++j)
                {
                    if (i * m_block_size + j >= OtherSize)
                    {
                        return;
                    }
                    m_data[i] |= arr[i * m_block_size + j] << j;
                }
            }
            if (m_partial_size)
            {
                for (uint16_t j = 0; j < m_partial_size; ++j)
                    m_data[m_storage_size - 1] |= static_cast<BlockType>(arr[m_storage_size - !!m_partial_size * m_block_size + j]) << j;
            }
        }

        /**
         * Bool pointer array to bitset conversion function
         * @tparam PtrArr Used to constrain type to const bool* [don't use]
         * @param ptr Pointer to the bool array containing the values to initialize the bitset with (bit values). It's size must be >= size()
         */
        template <typename PtrArr = const bool*> requires (std::is_pointer_v<PtrArr> && !std::is_array_v<PtrArr>&& std::is_same_v<const bool*, PtrArr> && !std::is_same_v<std::remove_cvref_t<BlockType>, bool*>)
        void _from_bool_array(PtrArr ptr) noexcept
        {
            for (size_t i = 0; i < m_storage_size - !!m_partial_size; ++i)
            {
                for (uint16_t j = 0; j < m_block_size; ++j)
                    m_data[i] |= *ptr++ << j;
            }
            if (m_partial_size)
            {
                for (uint16_t j = 0; j < m_partial_size; ++j)
                    m_data[m_storage_size - 1] |= static_cast<BlockType>(*ptr++) << j;
            }
        }

    public:

        /**
         * bitset to bool array conversion function
         * @return Pointer to array of booleans containing the bit values (size of the array == m_size template parameter)
         */
        [[nodiscard]] bool* to_bool_array() const noexcept
        {
            bool* result = new bool[m_size];

            for (size_t i = 0; i < m_storage_size - !!m_partial_size; ++i)
            {
                for (uint16_t j = 0; j < m_block_size; ++j)
                    *(result + i * m_block_size + j) = m_data[i] & BlockType{ 1 } << j;
            }
            if (m_partial_size)
            {
                for (size_t i = 0; i < m_partial_size; ++i)
                    *(result + (m_storage_size - 1) * m_block_size + i) = m_data[i] & BlockType{ 1 } << i;
            }
            return result;
        }

    private:

        /**
         * Bool container to bitset conversion function
         * @tparam T Type of the container to convert from [value_type equal to bool, support const_iterator]
         * @param cont Container containing bool values to convert from
         */
        template <has_read_iterator T> requires std::is_same_v<bool, typename T::value_type>
        void _from_bool_container(const T& cont) noexcept
        {
            typename T::const_iterator it = cont.cbegin();

            for (size_t i = 0; i < m_storage_size; ++i)
            {
                for (size_t j = 0; j < m_block_size; ++j)
                {
                    if (it == cont.cend())
                    {
                        return;
                    }
                    m_data[i] |= static_cast<BlockType>(*it++) << j;
                }
            }
        }

        /**
         * Bool container to bitset conversion function
         * @tparam T Type of the container to convert from [value_type equal to bool, support read bracket operator]
         * @param cont Container containing bool values to convert from
         */
        template <has_read_bracket_operator T> requires (std::is_same_v<bool, typename T::value_type> && !has_read_iterator<T>)
        void _from_bool_container(const T& cont) noexcept
        {
            typename T::const_iterator it = cont.cbegin();

            for (size_t i = 0; i < m_storage_size; ++i)
            {
                for (size_t j = 0; j < m_block_size; ++j)
                {
                    if (it == cont.cend())
                    {
                        return;
                    }
                    m_data[i] |= static_cast<BlockType>(*it++) << j;
                }
            }
        }

    public:

        /**
         * integral value to bitset conversion function
         * @tparam T Type of the integral value to convert from
         * @param value Value to convert from
         */
        template <unsigned_integer T>
        void from_integer(const T& value) noexcept
        {
            clear();
            if (sizeof(T) <= sizeof(BlockType))
            {
                m_data[0] = m_data[0] & ~static_cast<BlockType>((std::numeric_limits<T>::max)()) | value;
                return;
            }

            constexpr uint16_t diff = sizeof(T) / sizeof(BlockType);

            for (uint16_t i = 0; i < diff; ++i)
            {
                m_data[i] = value >> i * m_block_size;
            }
        }

        /**
         * bitset to integral value conversion function
         * @tparam T Type of the integral value to convert to
         * @return Converted integer value.
         */
        template <unsigned_integer T>
        [[nodiscard]] T to_integer() const noexcept
        {
            if (sizeof(T) <= sizeof(BlockType))
                return static_cast<T>(m_data[0]);

            constexpr uint16_t diff = sizeof(T) / sizeof(BlockType);
            T result = 0;
            for (int16_t i = 0; i < diff; ++i)
                result |= static_cast<T>(m_data[i]) << i * m_block_size;

            return result;
        }

        // Utility functions

        /**
         * Returns current dynamic_bitset instance as const (use when iterating, using [] operator, ...)
         */
    	[[nodiscard]] const dynamic_bitset& as_const() const noexcept
        {
            return *this;
        }

        /**
         * Swaps the values of two bits
         * @param index_1 Index of first value to swap (bit index)
         * @param index_2 Index of second value to swap (bit index)
         */
        void swap(const size_t& index_1, const size_t& index_2) noexcept
        {
            const bool tmp = test(index_1);
            set(index_1, test(index_2));
            set(index_2, tmp);
        }

        /**
         * Reverses the bitset (bit values)
         */
        void reverse() noexcept
        {
            for (size_t i = 0; i < m_size / 2; ++i)
            {
                swap(i, m_size - i - 1);
            }
        }

        /**
         * Rotates each bit to the left by the specified amount
         * @param shift Amount of bits to rotate to the left (bit count)
         */
        void rotate(const size_t& shift) noexcept
        {
            const dynamic_bitset tmp_cpy = *this;
            for (size_t i = 0; i < m_size; ++i)
                set(i, tmp_cpy.test((i + shift) % m_size));
        }

        /**
         * @return m_size of the bitset (bit count)
         */
        [[nodiscard]] size_t& size() noexcept { return m_size; }


        /**
         * @return m_size of the bitset (bit count)
         */
        [[nodiscard]] const size_t& size() const noexcept { return m_size; }

        /**
         * @return Number of blocks in the bitset
         */
        [[nodiscard]] size_t& storage_size() noexcept { return m_storage_size; }

        /**
		 * @return Number of blocks in the bitset
		 */
        [[nodiscard]] const size_t& storage_size() const noexcept { return m_storage_size; }

        /**
         * @return Pointer to the underlying array
         */
        [[nodiscard]] BlockType*& data() noexcept { return m_data; }

        /**
         * @return Const pointer to the underlying array
         */
        [[nodiscard]] const BlockType* const& data() const noexcept { return m_data; }

        // Iteration functions

        [[nodiscard]] iterator begin() noexcept
        {
            return iterator(*this);
        }

        [[nodiscard]] iterator end() noexcept
        {
            return iterator(*this, m_size);
        }

        [[nodiscard]] const_iterator begin() const noexcept
        {
            return const_iterator(*this);
        }

        [[nodiscard]] const_iterator end() const noexcept
        {
            return const_iterator(*this, m_size);
        }

        [[nodiscard]] const_iterator cbegin() const noexcept
        {
            return const_iterator(*this);
        }

        [[nodiscard]] const_iterator cend() const noexcept
        {
            return const_iterator(*this, m_size);
        }

        [[nodiscard]] reverse_iterator rbegin() noexcept
        {
            return reverse_iterator(*this, m_size - 1);
        }

        [[nodiscard]] reverse_iterator rend() noexcept
        {
            return reverse_iterator(*this, -1);
        }

        [[nodiscard]] const_reverse_iterator crbegin() const noexcept
        {
            return const_reverse_iterator(*this);
        }

        [[nodiscard]] const_reverse_iterator crend() const noexcept
        {
            return const_reverse_iterator(*this, -1);
        }

        [[nodiscard]] const_reverse_iterator rbegin() const noexcept
        {
            return const_reverse_iterator(*this);
        }

        [[nodiscard]] const_reverse_iterator rend() const noexcept
        {
            return const_reverse_iterator(*this, -1);
        }

        // bitset operations

        /**
         * Sets the bit at the specified index to the specified value
         * @param value Value to set the bit to (bit value)
         * @param index Index of the bit to set (bit index)
         */
        void set(const size_t& index, const bool& value = true) noexcept
        {
            if (value)
                m_data[index / m_block_size] |= BlockType{ 1 } << index % m_block_size;
            else
                m_data[index / m_block_size] &= ~(BlockType{ 1 } << index % m_block_size);
        }

        /**
         * Sets the bit at the specified index to 0 (false)
         * @param index Index of the bit to clear (bit index)
         */
        void clear(const size_t& index) noexcept
        {
            m_data[index / m_block_size] &= ~(BlockType{ 1 } << index % m_block_size);
        }

        /**
         * Fills all the bits with the specified value
         * @param value Value to fill the bits with (bit value)
         */
        void fill(const bool& value) noexcept
        {
            ::memset(m_data, value ? (std::numeric_limits<BlockType>::max)() : 0, m_storage_size * sizeof(BlockType));
        }

        /**
         * Fills all the bits with 1 (true)
         */
        void set() noexcept
        {
            ::memset(m_data, 255u, m_storage_size * sizeof(BlockType));
        }

        /**
         * Resets all the bits (sets all bits to 0)
         */
        void clear() noexcept
        {
            ::memset(m_data, 0, m_storage_size * sizeof(BlockType));
        }

        /**
         * Fills all the bits in the specified range with the specified value
         * @param value Value to fill the bits with (bit value)
         * @param end End of the range to fill (bit index)
         */
        void fill(const size_t& end, const bool& value) noexcept
        {
            ::memset(m_data, value ? 255u : 0, end / m_block_size * sizeof(BlockType));

            if (end % m_block_size)
            {
                if (value)
                {
                    for (uint16_t i = 0; i < end % m_block_size; ++i)
                        m_data[end / m_block_size] |= BlockType{ 1 } << i;
                }
                else if (!value)
                {
                    for (uint16_t i = 0; i < end % m_block_size; ++i)
                        m_data[end / m_block_size] &= ~(BlockType{ 1 } << i);
                }
            }
        }

        /**
         * Fills all the bits in the specified range with the specified value
         * @param end End of the range to set (bit index)
         */
        void set_range(const size_t& end) noexcept
        {
            ::memset(m_data, 255u, end / m_block_size * sizeof(BlockType));

            if (end % m_block_size)
            {
                for (uint16_t i = 0; i < end % m_block_size; ++i)
                    m_data[end / m_block_size] |= BlockType{ 1 } << i;
            }
        }

        /**
         * Fills all the bits in the specified range with 0 (false)
         * @param end End of the range to set (bit index)
         */
        void clear_range(const size_t& end) noexcept
        {
            ::memset(m_data, 0, end / m_block_size * sizeof(BlockType));

            if (end % m_block_size)
            {
                for (uint16_t i = 0; i < end % m_block_size; ++i)
                    m_data[end / m_block_size] &= ~(BlockType{ 1 } << i);
            }
        }

        /**
         * Fills all the bits in the specified range with the specified value
         * @param value Value to fill the bits with (bit value)
         * @param begin Begin of the range to fill (bit index)
         * @param end End of the range to fill (bit index)
         */
        void fill_range(const size_t& begin, const size_t& end, const bool& value) noexcept
        {
            uint8_t to_add = 1, to_sub = 1;
            // create begin_block and fill the first byte with it
            if (begin % m_block_size)
            {
                const uint16_t end_bit = (begin / m_block_size == end / m_block_size) ? end % m_block_size : m_block_size;
                if (value)
                {
                    for (uint16_t i = begin % m_block_size; i < end_bit; ++i)
                        m_data[begin / m_block_size] |= BlockType{ 1 } << i;
                }
                else
                {
                    for (uint16_t i = begin % m_block_size; i < end_bit; ++i)
                        m_data[begin / m_block_size] &= ~(BlockType{ 1 } << i);
                }
            }
            else
                to_add = 0;

            // set the end block if the end is not aligned with the block size
            if (end % m_block_size && begin / m_block_size != end / m_block_size)
            {
                if (value)
                {
                    for (uint16_t i = 0; i < end % m_block_size; ++i)
                        m_data[end / m_block_size] |= BlockType{ 1 } << i;
                }
                else
                {
                    for (uint16_t i = 0; i < end % m_block_size; ++i)
                        m_data[end / m_block_size] &= ~(BlockType{ 1 } << i);
                }
            }
            else
                to_sub = 0;

            ::memset(m_data + begin / m_block_size + to_add, value ? 255u : 0, (end - begin) / m_block_size * sizeof(BlockType) - to_sub);
        }

        /**
         * Fills all the bits in the specified range with 0 (false)
         * @param begin Begin of the range to fill (bit index)
         * @param end End of the range to fill (bit index)
         */
        void set_range(const size_t& begin, const size_t& end) noexcept
        {
            uint8_t to_add = 1, to_sub = 1;
            // create begin_block and fill the first byte with it
            if (begin % m_block_size)
            {
                const uint16_t end_bit = begin / m_block_size == end / m_block_size ? end % m_block_size : m_block_size;
                for (uint16_t i = begin % m_block_size; i < end_bit; ++i)
                    m_data[begin / m_block_size] |= BlockType{ 1 } << i;
            }
            else
                to_add = 0;

            // set the end block if the end is not aligned with the block size
            if (end % m_block_size && begin / m_block_size != end / m_block_size)
            {
                for (uint16_t i = 0; i < end % m_block_size; ++i)
                    m_data[end / m_block_size] |= BlockType{ 1 } << i;
            }
            else
                to_sub = 0;

            ::memset(m_data + begin / m_block_size + to_add, 255u, (end - begin) / m_block_size * sizeof(BlockType) - to_sub);
        }

        /**
         * Fills all the bits in the specified range with 0 (false)
         * @param begin Begin of the range to fill (bit index)
         * @param end End of the range to fill (bit index)
         */
        void clear_range(const size_t& begin, const size_t& end) noexcept
        {
            uint8_t to_add = 1, to_sub = 1;
            // create begin_block and fill the first byte with it
            if (begin % m_block_size)
            {
                const uint16_t end_bit = begin / m_block_size == end / m_block_size ? end % m_block_size : m_block_size;
                for (uint16_t i = begin % m_block_size; i < end_bit; ++i)
                    m_data[begin / m_block_size] &= ~(BlockType{ 1 } << i);
            }
            else
                to_add = 0;

            // set the end block if the end is not aligned with the block size
            if (end % m_block_size && begin / m_block_size != end / m_block_size)
            {
                for (uint16_t i = 0; i < end % m_block_size; ++i)
                    m_data[end / m_block_size] &= ~(BlockType{ 1 } << i);
            }
            else
                to_sub = 0;

            ::memset(m_data + begin / m_block_size + to_add, 0, (end - begin) / m_block_size * sizeof(BlockType) - to_sub);
        }

        /**
         * Fills all the bits in the specified range with the specified value
         * @param value Value to fill the bits with (bit value)
         * @param begin Begin of the range to fill (bit index)
         * @param end End of the range to fill (bit index)
         * @param step Step size between the bits to fill (bit step)
         */
        void fill_range(const size_t& begin, const size_t& end, const size_t& step, const bool& value) noexcept
        {
            for (size_t i = begin; i < end; i += step)
            {
                if (value)
                    m_data[i / m_block_size] |= BlockType{ 1 } << i % m_block_size;
                else
                    m_data[i / m_block_size] &= ~(BlockType{ 1 } << i % m_block_size);
            }
        }

        /**
         * Fills all the bits in the specified range with 0 (false)
         * @param begin Begin of the range to fill (bit index)
         * @param end End of the range to fill (bit index)
         * @param step Step size between the bits to fill (bit step)
         */
        void set_range(const size_t& begin, const size_t& end, const size_t& step) noexcept
        {
            for (size_t i = begin; i < end; i += step)
                m_data[i / m_block_size] |= BlockType{ 1 } << i % m_block_size;
        }

        /**
         * Fills all the bits in the specified range with 0 (false)
         * @param begin Begin of the range to fill (bit index)
         * @param end End of the range to fill (bit index)
         * @param step Step size between the bits to fill (bit step)
         */
        void clear_range(const size_t& begin, const size_t& end, const size_t& step) noexcept
        {
            for (size_t i = begin; i < end; i += step)
                m_data[i / m_block_size] &= ~(BlockType{ 1 } << i % m_block_size);
        }

        /**
         * !!! W.I.P. - Does not function correctly at the moment !!!\n
         * Fill the bits in the specified range with the specified value using an optimized algorithm.\n
         * This algorithm is particularly efficient when the step size is relatively low.\n
         * Note: This function has a rather complex implementation. It is not recommended to use it when simple filling without a step is possible.\n
         * Performance of this function varies significantly depending on the step. It performs best when step is a multiple of m_block_size, and is within reasonable range from it.\n
         * However, worst when step is not aligned with m_block_size and end is not aligned with m_block_size. In such cases, extra processing is required to handle the boundary blocks.\n
         * @param value Value to fill the bits with (bit value)
         * @param begin Begin of the range to fill (bit index)
         * @param end End of the range to fill (bit index)
         * @param step Step size between the bits to fill (bit step)
         */
        void fill_range_optimized(const size_t& begin, const size_t& end, const size_t& step, const bool& value) noexcept
        {
            // Initialize variables
            size_t blocks_size, current_block = begin / m_block_size + 1 + step / m_block_size, current_offset = 0;
            uint16_t offset;
            const size_t end_block = end / m_block_size + (end % m_block_size ? 1 : 0);

            // Determine the size of blocks based on step and block size
            if ((step % 2 || step <= m_block_size) && m_block_size % step) {
                blocks_size = (std::min)(m_storage_size, step);
            }
            else if (!(m_block_size % step))
                blocks_size = 1;
            else if (!(step % m_block_size))
                blocks_size = step / m_block_size;
            else
            {
                // GCD of step and m_block_size
                if (step % m_block_size)
                {
                    size_t a = step, b = m_block_size, t = m_block_size;
                    while (b) {
                        t = b;
                        b = a % b;
                        a = t;
                    }
                    blocks_size = a;
                }
                else
                    blocks_size = 1;
            }

            // Calculate the offset
            if (begin < m_block_size)
            {
                offset = (m_block_size - begin) % step;
                if (offset)
                    offset = step - offset;
            }
            else
            {
                offset = (begin - m_block_size) % step;
            }

            if (offset)
                offset = (m_block_size - offset + step / m_block_size * m_block_size) % step;

            std::cout << "offset: " << offset << '\n';

            // Create and apply the beginning block
            {
                const uint16_t end_bit = (begin / m_block_size == end / m_block_size) ? end % m_block_size : m_block_size;
                if (value)
                {
                    for (uint16_t i = begin % m_block_size; i < end_bit; i += step)
                        m_data[begin / m_block_size] |= BlockType{ 1 } << i;
                }
                else
                {
                    for (uint16_t i = begin % m_block_size; i < end_bit; i += step)
                        m_data[begin / m_block_size] &= ~(BlockType{ 1 } << i);
                }
            }


            // Fill with appropriate block
            std::cout << blocks_size << " blocks\n";
            std::cout << (std::min)(blocks_size + begin / m_block_size, m_storage_size) << " blocks\n";
            for (size_t i = 0; i < (std::min)(blocks_size, m_storage_size); ++i)
            {
                // Generate block for the current iteration
                BlockType block = 0;

                if (value)
                {
                    for (uint16_t j = !i ? offset : 0; j < m_block_size; ++j)
                    {
                        std::cout << current_block * m_block_size + j - offset << '\n';
                        if (!((current_block * m_block_size + j - offset) % step))
                            block |= BlockType{ 1 } << j;
                    }
                }
                else
                {
                    block = (std::numeric_limits<BlockType>::max)();
                    for (uint16_t j = (!i ? offset : 0); j < m_block_size; ++j)
                    {
                        if (!((current_block * m_block_size + j - offset) % step))
                            block &= ~(BlockType{ 1 } << j);
                    }
                }

                // print the block
                for (uint16_t j = 0; j < m_block_size; ++j)
                {
                    std::cout << ((block & BlockType{ 1 } << j) >> j);
                }

                std::cout << '\n';

                // Apply the block
                for (size_t j = current_block; j < m_storage_size; ++j)
                {
                    if (j == end_block - 1 && end % m_block_size)
                    {
                        // Remove bits that overflow the range
                        if (value)
                        {
                            for (uint16_t k = end % m_block_size; k < m_block_size; ++k)
                                block &= ~(BlockType{ 1 } << k);
                            *(m_data + j) |= block;
                        }
                        else
                        {
                            for (uint16_t k = end % m_block_size; k < m_block_size; ++k)
                                block |= BlockType{ 1 } << k;
                            *(m_data + j) &= block;
                        }
                        break;
                    }
                    if (value)
                        *(m_data + j) |= block;
                    else
                        *(m_data + j) &= block;
                }
                ++current_block;
            }
        }

        /**
         * !!! W.I.P. - May not choose the best option, not even talking about the fact that set_range_optimized function doesn't even work correctly !!!\n
         * Fill the bits in the specified range with the specified value.\n
         * Chooses the fastest implementation based on the step.\n
         * This function becomes more accurate in choosing the fastest implementation as the size of the bitset increases.\n
         * @param value Value to fill the bits with
         * @param begin Begin of the range to fill (bit index)
         * @param end End of the range to fill (bit index)
         * @param step Step size between the bits to fill
        */
        void fill_range_fastest(const size_t& begin, const size_t& end, const size_t& step, const bool& value) noexcept
        {
            if (step == 1)
            {
                fill_range(begin, end, value);
                return;
            }
            if (step <= m_block_size * 2.5) // approximately up until this point it is faster, though no scientific anything went into this, just a guess lol
            {
                fill_range_optimized(value, begin, end, step);
                return;
            }
            fill_range(value, begin, end, step);
        }

        /**
         * Sets the block at the specified index to the specified value (default is max value, all bits set to 1)
         * @param block Chunk to set (block value)
         * @param index Index of the block to set (block index)
         */
        void set_block(const size_t& index, const BlockType& block = (std::numeric_limits<BlockType>::max)()) noexcept
        {
            m_data[index] = block;
        }

        /**
         * Sets the block at the specified index to 0 (all bits set to 0)
         * @param index Index of the block to clear (block index)
         */
        void clear_block(const size_t& index) noexcept
        {
            m_data[index] = 0u;
        }

        /**
         * Fills all the blocks with the specified block
         * @param block Chunk to fill the blocks with (block value)
         */
        void fill_block(const BlockType& block) noexcept
        {
            for (size_t i = 0; i < m_storage_size; ++i)
                m_data[i] = block;
        }

        /**
         * Fills all the bits in the specified range with the specified block
         * @param block Chunk to fill the bits with (block value)
         * @param end End of the range to fill (block index)
         */
        void fill_block_range(const size_t& end, const BlockType& block = (std::numeric_limits<BlockType>::max)()) noexcept
        {
            for (size_t i = 0; i < end; ++i)
                m_data[i] = block;
        }

        /**
         * Fills all the bits in the specified range with the specified block
         * @param block Chunk to fill the bits with
         * @param begin begin of the range to fill (block index)
         * @param end End of the range to fill (block index)
         */
        void fill_block_range(const size_t& begin, const size_t& end, const BlockType& block = (std::numeric_limits<BlockType>::max)()) noexcept
        {
            for (size_t i = begin; i < end; ++i)
                m_data[i] = block;
        }

        /**
         * Fills all the bits in the specified range with the specified block
         * @param block Chunk to fill the bits with (block value)
         * @param begin begin of the range to fill (block index)
         * @param end End of the range to fill (block index)
         * @param step Step size between the bits to fill (block step)
         */
        void fill_block_range(const size_t& begin, const size_t& end, const size_t& step, const BlockType& block = (std::numeric_limits<BlockType>::max)()) noexcept
        {
            for (size_t i = begin; i < end; i += step)
                m_data[i] = block;
        }

        /**
         * Flips the bit at the specified index
         * @param index Index of the bit to flip (bit index)
         */
        void flip(const size_t& index) noexcept
        {
            m_data[index / m_block_size] ^= BlockType{ 1 } << index % m_block_size;
        }

        /**
         * Flips all the bits
         */
        void flip() noexcept
        {
            for (size_t i = 0; i < m_storage_size; ++i)
                m_data[i] = ~m_data[i];
        }

        /**
         * Flips all the bits in the specified range
         * @param end End of the range to fill (bit index)
         */
        void flip_range(const size_t& end) noexcept
        {
            // flip blocks that are in range by bulk, rest flip normally
            for (size_t i = 0; i < end / m_block_size; ++i)
                m_data[i] = ~m_data[i];
            for (uint16_t i = 0; i < end % m_block_size; ++i)
                m_data[end / m_block_size] ^= BlockType{ 1 } << i;
        }

        /**
         * Flip all the bits in the specified range
         * @param begin Begin of the range to fill (bit index)
         * @param end End of the range to fill (bit index)
         */
        void flip_range(const size_t& begin, const size_t& end) noexcept
        {
            size_t to_add = 1;
            if (begin % m_block_size)
            {
                for (uint16_t i = begin % m_block_size; i < m_block_size; ++i)
                    m_data[begin / m_block_size] ^= BlockType{ 1 } << i;
            }
            else
                to_add = 0;

            for (size_t i = begin / m_block_size + to_add; i < end / m_block_size; ++i)
                m_data[i] = ~m_data[i];

            if (end % m_block_size)
            {
                for (uint16_t i = 0; i < end % m_block_size; ++i)
                    m_data[end / m_block_size] ^= BlockType{ 1 } << i;
            }
        }

        /**
         * Flips all the bits in the specified range
         * @param begin Begin of the range to fill (bit index)
         * @param end End of the range to fill (bit index)
         * @param step Step size between the bits to flip (bit step)
         */
        void flip_range(const size_t& begin, const size_t& end, const size_t& step) noexcept
        {
            for (size_t i = begin; i < end; i += step)
            {
                m_data[i / m_block_size] ^= BlockType{ 1 } << i % m_block_size;
            }
        }

        /**
         * Flips the block at the specified index
         * @param index Index of the block to flip (block index)
         */
        void flip_block(const size_t& index) noexcept
        {
            m_data[index] = ~m_data[index];
        }

        /**
         * Flips all the blocks in the specified range
         * @param end End of the range to fill (block index)
         */
        void flip_block_range(const size_t& end) noexcept
        {
            for (size_t i = 0; i < end; ++i)
                m_data[i] = ~m_data[i];
        }

        /**
         * Flips all the blocks in the specified range
         * @param begin Begin of the range to fill (block index)
         * @param end End of the range to fill (block index)
         */
        void flip_block_range(const size_t& begin, const size_t& end) noexcept
        {
            for (size_t i = begin; i < end; ++i)
                m_data[i] = ~m_data[i];
        }

        /**
         * Flips all the blocks in the specified range
         * @param begin Begin of the range to fill (block index)
         * @param end End of the range to fill (block index)
         * @param step Step size between the bits to flip (block step)
         */
        void flip_block_range(const size_t& begin, const size_t& end, const size_t& step) noexcept
        {
            for (size_t i = begin; i < end; i += step)
                m_data[i] = ~m_data[i];
        }

        /**
         * Retrieves the value of a bit at a specified index
         * @param index The index of the bit to read (bit index)
         * @return The value of the bit at the specified index
         */
        [[nodiscard]] bool test(const size_t& index) const noexcept
        {
            return m_data[index / m_block_size] & BlockType{ 1 } << index % m_block_size;
        }

        /**
         * Retrieves the block at the specified index
         * @param index Index of the block to retrieve (block index)
         * @return Chunk at the specified index
         */
        [[nodiscard]] const BlockType& get_block(const size_t& index) const noexcept
        {
            return m_data[index];
        }

        /**
         * Retrieves the block at the specified index
         * @param index Index of the block to retrieve (block index)
         * @return Chunk at the specified index
         */
        [[nodiscard]] BlockType& get_block(const size_t& index) noexcept
        {
            return m_data[index];
        }


        /**
         * Checks if all bits are set
         * @return true if all bits are set, false otherwise
         */
        [[nodiscard]] bool all() const noexcept
        {
            // check all except the last one if the size is not divisible by m_block_size
            for (size_t i = 0; i < m_storage_size - !!m_partial_size; ++i)
            {
                if (m_data[i] != (std::numeric_limits<BlockType>::max)())
                    return false;
            }
            if (m_partial_size)
            {
                for (uint16_t i = 0; i < m_partial_size; ++i)
                {
                    if (!(m_data[m_size / m_block_size] & BlockType{ 1 } << i))
                        return false;
                }
            }
            return true;
        }

        /**
         * Checks if any bit is set
         * @return true if any bit is set, false otherwise
         */
        [[nodiscard]] bool any() const noexcept
        {
            for (size_t i = 0; i < m_storage_size - !!m_partial_size; ++i)
            {
                if (m_data[i])
                    return true;
            }
            if (m_partial_size)
            {
                for (uint16_t i = 0; i < m_partial_size; ++i)
                {
                    if (m_data[m_size / m_block_size] & BlockType{ 1 } << i)
                        return true;
                }
            }
            return false;
        }

        /**
         * Checks if none of the bits are set
         * @return true if none of the bits are set, false otherwise
         */
        [[nodiscard]] bool none() const noexcept
        {
            for (size_t i = 0; i < m_storage_size - !!m_partial_size; ++i)
            {
                if (m_data[i])
                    return false;
            }
            if (m_partial_size)
            {
                for (uint16_t i = 0; i < m_partial_size; ++i)
                {
                    if (m_data[m_size / m_block_size] & BlockType{ 1 } << i)
                        return false;
                }
            }
            return true;
        }

        /**
         * Checks if all bits are cleared (none are set)
         * @return true if all bits are cleared, false otherwise
         */
        [[nodiscard]] bool all_clear() const noexcept
        {
            return none();
        }

        /**
         * @return The number of set bits
         */
        [[nodiscard]] size_t count() const noexcept
        {
            size_t count = 0;
            for (size_t i = 0; i < m_storage_size; ++i)
            {
                BlockType j = m_data[i];
                while (j)
                {
                    j &= j - 1;
                    ++count;
                }
            }
            return count;
        }

        /**
         * Checks if the bitset is empty
         * @return true if the bitset is empty, false otherwise
         */
        [[nodiscard]] bool empty() const noexcept
        {
            return !m_size;
        }

        /**
         * Returns bitset's capacity in bits (capacity = number of bits that can be stored w/o reallocation)
         * @return 
         */
        [[nodiscard]] size_t capacity() const noexcept
        {
	        return m_storage_size * m_block_size;
		}

        /**
         * Resizes the bitset to the specified size, but does not define values in the expanded area
         * @param new_size New size of the bitset to resize to (bit count)
         */
        void resize(const size_t& new_size)
        {
            if (new_size == m_size)
                return;

            if (!new_size)
            {
                m_partial_size = m_storage_size = m_size = 0;
                if (m_data)
                {
	                delete[] m_data;
                }
                m_data = nullptr;
                return;
            }

            BlockType* new_data = new BlockType[new_size];
            if (m_data)
            {
                if (m_storage_size < new_size)
                {
                    std::copy(m_data, m_data + m_storage_size, new_data);
                    ::memset(new_data + m_storage_size, 0, (new_size - m_storage_size) * sizeof(BlockType)); // ensure 0 initialization
                }
                else
                {
                	std::copy(m_data, m_data + new_size, new_data);
                }
                delete[] m_data;
            }
            m_partial_size = new_size % m_block_size;
            m_storage_size = new_size / m_block_size + !!m_partial_size;
            m_size = new_size;
            m_data = new_data;
        }

        /**
		 * Pushes back bit value to the bitset
		 * @param value Value to push back (bit value)
		 */
        void push_back(const bool& value)
        {
            if (!(m_size++ % m_block_size))
            {
                m_partial_size = 0;
                BlockType* new_data = new BlockType[++m_storage_size];
                if (m_data)
                {
                    std::copy(m_data, m_data + m_storage_size, new_data);
                    delete[] m_data;
                }
                *(new_data + m_storage_size - 1) = 0;
                m_data = new_data;
            }
            else
            {
                ++m_partial_size;
            }
            if (value)
				m_data[m_storage_size - 1] |= BlockType{ 1 } << m_partial_size - 1;
            else
				m_data[m_storage_size - 1] &= ~(BlockType{ 1 } << m_partial_size - 1);
        }

        /**
		 * Pops back bit value from the bitset
		 */
        void pop_back()
        {
            if (!(--m_size % m_block_size))
            {
                m_partial_size = 0;
                --m_storage_size;
                if (m_size)
                {
                    BlockType* new_data = new BlockType[m_storage_size];
                    std::copy(m_data, m_data + m_storage_size, new_data);
                    delete[] m_data;
                    m_data = new_data;
                }
                else
                {
                    delete[] m_data;
                    m_data = nullptr;
                }
                --m_size;
            }
            else
            {
                --m_partial_size;
            }
        }

        /**
         * Inserts bit value at the specified index
         * When inserting at the last position, for more info see push_back
         * @param index Index to insert the value at (bit index)
         * @param value Value to insert (bit value)
         */
        void insert(const size_t& index, const bool& value)
        {
            if (index == m_size)
            {
                push_back(value);
                return;
            }

            if (!(m_size % m_block_size))
            {
                m_partial_size = 0;
                BlockType* new_data = new BlockType[++m_storage_size];
                if (m_data)
                {
                    // copy everything prepending the bit
                    for (size_t i = 0; i < index; ++i)
                    {
                        if (test(i))
                            *(new_data + i / m_block_size) |= BlockType{ 1 } << i % m_block_size;
                        else
                            *(new_data + i / m_block_size) &= ~(BlockType{ 1 } << i % m_block_size);
                    }
                    // copy everything appending the bit
                    for (size_t i = index; i < m_size; ++i)
                    {
                        if (test(i))
                            *(new_data + (i + 1) / m_block_size) |= BlockType{ 1 } << (i + 1) % m_block_size;
                        else
                            *(new_data + (i + 1) / m_block_size) &= ~(BlockType{ 1 } << (i + 1) % m_block_size);
                    }
                    delete[] m_data;
                    m_data = new_data;
                    set(index, value);
                }
                m_data = new_data;
            }
            else
            {
                const dynamic_bitset tmp_copy(*this);
                // copy everything appending the bit
                for (size_t i = index; i < m_size; ++i)
                {
                    set(i + 1, tmp_copy.test(i));
                }
                set(index, value);
                ++m_partial_size;
            }
            ++m_size;
        }

        /**
		 * Pushes back block value to the bitset. \n
		 * If bitset's size is not a multiple of/divisible by block size, \n
		 * additionally fully expands the not fully utilized block, \n
		 * although values in the expanded area are in valid, but unspecified state by this function \n
		 * e.g. (BlockType=uint64_t, m_size=65, push_back_block call -> {m_size 65 -> 128 [expansion of not fully utilized block] -> 192 [expansion to hold 1 additional block]})
		 * @param block Block value to push back (block value)
		 */
        void push_back_block(const BlockType& block)
        {
            m_size = ++m_storage_size * m_block_size;
            m_partial_size = 0;
            BlockType* new_data = new BlockType[m_storage_size];
            if (m_data)
            {
                std::copy(m_data, m_data + m_storage_size, new_data);
                delete[] m_data;
            }
            m_data = new_data;
            *(m_data + m_storage_size - 1) = block;
        }

        /**
         * Pops back block value from the bitset. \n
         * If bitset's size is not a multiple of/divisible by block size, \n
         * removes the not fully utilized block first \n
         * e.g. (BlockType=uint64_t, m_size=65, pp_back_block call -> {m_size 65 -> 64 [removal of last, not fully utilized block]}
         */
        void pop_back_block()
        {
            m_size = --m_storage_size * m_block_size;
            m_partial_size = 0;
            BlockType* new_data = new BlockType[m_storage_size];
            if (m_data)
            {
                std::copy(m_data, m_data + m_storage_size, new_data);
                delete[] m_data;
            }
            m_data = new_data;
        }

        /**
		 * Inserts block value at the specified index to the bitset.\n
		 * When inserting at the last position, for more info see push_back_block
		 * @param index Index to insert the value at (block index)
		 * @param block Block value to insert (block value)
		 */
        void insert_block(const size_t& index, const BlockType& block)
        {
            if (index == m_storage_size)
            {
	            push_back_block(block);
                return;
            }

            BlockType* new_data = new BlockType[m_storage_size + 1];
            if (m_data)
            {
                // copy everything prepending the block
                for (size_t i = 0; i < index; ++i)
                    *(new_data + i) = *(m_data + i);

                // copy everything appending the block
                for (size_t i = index; i < m_storage_size; ++i)
                    *(new_data + i + 1) = *(m_data + i);
                delete[] m_data;
            }
            
            m_size += m_block_size;
            ++m_storage_size;
        	m_data = new_data;

            *(new_data + index) = block;
        }

        /**
         * Bit-length of the underlying type
         */
        static constexpr uint16_t m_block_size = sizeof(BlockType) * 8;

        /**
         * Count of bits that are utilized in last dangling block, if any
         */
        uint8_t m_partial_size;

        /**
         * m_size of the bitset in blocks
         */
        size_t m_storage_size;

        /**
         * m_size of the bitset in bits
         */
        size_t m_size;

        /**
         * Underlying array of blocks containing the bits
         */
         alignas(std::hardware_destructive_interference_size) BlockType* m_data;
    };
};


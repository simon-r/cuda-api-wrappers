#pragma once
#ifndef CUDA_TYPES_H_
#define CUDA_TYPES_H_

#ifndef __CUDACC__
#include <builtin_types.h>
#include <driver_types.h>
#endif

namespace cuda {

/*
 * The 3 ID types - for devices, streams and events - in this file are just
 * numeric identifiers (mostly useful for breaking dependencies and for
 * interaction with code using the original CUDA APIs); we also have wrapper
 * classes for them (each constructible by the corresponding numeric ID type)
 * which allow convenient access to their related functionality: these are
 * @ref cuda::device_t, @ref cuda::stream_t and @ref cuda::event_t.
 *
 * TODO: Perhaps we should call them cuda::device::type,
 * cuda::stream::type and cuda::event::type,
 */


// It's not an _error_ people!
using status_t                = cudaError_t;

using grid_dimension_t        = unsigned;
using grid_block_dimension_t  = unsigned;

namespace event {
using id_t              = cudaEvent_t;
} // namespace event

namespace stream {
using id_t             = cudaStream_t;
using priority_t       = int;
enum : priority_t { default_priority = 0 };

} // namespace stream

struct dimensions_t : public dim3 // it's 3 uints in a struct. But it's not uint3 !
{
	using dim3::dim3;
	dimensions_t(const dim3& dims) : dim3(dims) { }

	inline size_t empty() const
	{
		return (x == 0) or (y == 0) or  (z == 0);
	}
	inline size_t volume() const
	{
		return (size_t) x * y * z;
	}
	inline unsigned char dimensionality() const
	{
		return (z > 1) + (y > 1) + (x > 1);
	}
};

inline bool operator==(const dim3& lhs, const dim3& rhs)
{
   return lhs.x == rhs.x and lhs.y == rhs.y and lhs.z == rhs.z;
}
inline bool operator==(const dimensions_t& lhs, const dimensions_t& rhs)
{
   return static_cast<dim3>(lhs) == static_cast<dim3>(rhs);
}


using grid_dimensions_t       = dimensions_t;
using grid_block_dimensions_t = dimensions_t;
using shared_memory_size_t    = unsigned short;
	// Like size_t, but for shared memory spaces, which, currently in nVIDIA (and AMD)
	// GPUs are sized at no more than 64 KiB. Note that using this for computations
	// might not be the best idea since there are penalties for sub-32-bit computation
	// sometimes

// TODO: Consider making this a non-POD struct,
// with a proper ctor checking validity, an operator converting to pair etc;
// however, that would require including at least std::utility, if not other
// stuff (e.g. for an std::hash specialization)
struct compute_capability_t {
	unsigned major;
	unsigned minor;

	unsigned as_combined_number() const { return major * 10 + minor; }
	unsigned max_warp_schedulings_per_processor_cycle();
	unsigned max_resident_warps_per_processor();
	unsigned max_in_flight_threads_per_processor();
	shared_memory_size_t max_shared_memory_per_block();
	const char* architecture_name();
	static const compute_capability_t from_combined_number(unsigned combined)
	{
		return  { combined / 10, combined % 10 };
	}
	bool is_valid() const {
		return (major > 0) and (major < 9999) and (minor > 0) and (minor < 9999);
	}
};

inline bool operator ==(const compute_capability_t& lhs, const compute_capability_t& rhs)
{
	return lhs.major == rhs.major and lhs.minor == rhs.minor;
}
inline bool operator !=(const compute_capability_t& lhs, const compute_capability_t& rhs)
{
	return lhs.major != rhs.major or lhs.minor != rhs.minor;
}
inline bool operator <(const compute_capability_t& lhs, const compute_capability_t& rhs)
{
	return lhs.major < rhs.major or (lhs.major == rhs.major and lhs.minor < rhs.minor);
}
inline bool operator <=(const compute_capability_t& lhs, const compute_capability_t& rhs)
{
	return lhs.major < rhs.major or (lhs.major == rhs.major and lhs.minor <= rhs.minor);
}
inline bool operator >(const compute_capability_t& lhs, const compute_capability_t& rhs)
{
	return lhs.major > rhs.major or (lhs.major == rhs.major and lhs.minor > rhs.minor);
}
inline bool operator >=(const compute_capability_t& lhs, const compute_capability_t& rhs)
{
	return lhs.major > rhs.major or (lhs.major == rhs.major and lhs.minor >= rhs.minor);
}

constexpr compute_capability_t make_compute_capability(unsigned major, unsigned minor)
{
	return { major, minor };
}
constexpr compute_capability_t make_compute_capability(unsigned combined)
{
	return { combined / 10, combined % 10 };
}

typedef struct {
	grid_dimensions_t       grid_dimensions;
	grid_block_dimensions_t block_dimensions;
	shared_memory_size_t    dynamic_shared_memory_size; // in bytes
} launch_configuration_t;

inline launch_configuration_t make_launch_config(
	grid_dimensions_t       grid_dimensions,
	grid_block_dimensions_t block_dimensions,
	shared_memory_size_t    dynamic_shared_memory_size = 0)
{
	return { grid_dimensions, block_dimensions, dynamic_shared_memory_size };
}

inline bool operator==(const launch_configuration_t lhs, const launch_configuration_t& rhs)
{
	return
		lhs.grid_dimensions    == rhs.grid_dimensions    and
		lhs.block_dimensions   == rhs.block_dimensions   and
		lhs.dynamic_shared_memory_size == rhs.dynamic_shared_memory_size;
}

enum class multiprocessor_cache_preference_t {
	no_preference                 = cudaFuncCachePreferNone,
	equal_l1_and_shared_memory    = cudaFuncCachePreferEqual,
	prefer_shared_memory_over_l1  = cudaFuncCachePreferShared,
	prefer_l1_over_shared_memory  = cudaFuncCachePreferL1,
	// aliases
	none                          = no_preference,
	equal                         = equal_l1_and_shared_memory,
	prefer_shared                 = prefer_shared_memory_over_l1,
	prefer_l1                     = prefer_l1_over_shared_memory,
};

enum class multiprocessor_shared_memory_bank_size_option_t {
    device_default       = 0,
    four_bytes_per_bank  = 1,
    eight_bytes_per_bank = 2
};

/**
 * This type is intended for kernel parameters and kernel template parameters
 * indicating how many elements of an input or output array a single thread
 * will do computational work for, rather than having each thread do work for
 * a single element. Thus for example when performing elementwise array addition
 * on arrays of length n, with serialization factor 1 we would use n threads,
 * while with serialization factor s we would use ceil(n/s) threads (ignoring
 * rounding up to the nearest multiple of the block size).
 */
using serialization_factor_t = unsigned short;

namespace device {

using id_t               = int;

using attribute_t        = enum cudaDeviceAttr;
	// ... not to be confused with the properties in properties_t above...
using attribute_value_t  = int;

using pair_attribute_t   = cudaDeviceP2PAttr;

// TODO: Try to drop this type, I don't think we really need it
using flags_t            = unsigned;

} // namespace device

namespace detail {
enum : bool { assume_device_is_current = true, do_not_assume_device_is_current = false };
}

enum host_thread_synch_scheduling_policy_t : unsigned int {
	heuristic = cudaDeviceScheduleAuto,
    spin      = cudaDeviceScheduleSpin,
    yield     = cudaDeviceScheduleYield,
    block     = cudaDeviceScheduleBlockingSync,
};

enum synchronicity_t : bool {
	asynchronous = false,
	synchronous = true,
	sync = synchronous,
	async = asynchronous,
};

enum class endianness_t : bool { big, big_endian = big, little, little_endian = little };

} // namespace cuda

#endif /* CUDA_TYPES_H_ */
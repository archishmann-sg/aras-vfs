module;
#include <cstdint>
#include <string>
export module vfs:types;

import core;

export namespace vfs::inline types
{
    // Exported strong type tags
    namespace tag
    {
        struct vfs_stat_size;
        struct vfs_stat_creation_time;
        struct vfs_stat_modification_time;
        struct vfs_stat_access_time;
        struct vfs_stat_permissions;

        struct volume_total_space;
        struct volume_free_space;

        struct file_handle_id;
        struct offset;
    } // namespace tag

    using vfs_stat_size_t = core::c_strong_type<std::uint64_t, tag::vfs_stat_size>;
    using vfs_stat_creation_time_t = core::c_strong_type<std::uint64_t, tag::vfs_stat_creation_time>;
    using vfs_stat_modification_time_t = core::c_strong_type<std::uint64_t, tag::vfs_stat_modification_time>;
    using vfs_stat_access_time_t = core::c_strong_type<std::uint64_t, tag::vfs_stat_access_time>;
    using vfs_stat_permissions_t = core::c_strong_type<std::uint32_t, tag::vfs_stat_permissions>;

    using volume_total_space_t = core::c_strong_type<std::uint64_t, tag::volume_total_space>;
    using volume_free_space_t = core::c_strong_type<std::uint64_t, tag::volume_free_space>;

    using file_handle_id_t = core::c_strong_type<std::uint64_t, tag::file_handle_id>;
    using offset_t = core::c_strong_type<std::uint64_t, tag::offset>;

    namespace err_code
    {
        // TODO: Define error codes for interface.

    } // namespace err_code

    // Standard access request bitmask
    enum class e_open_mode : std::uint8_t
    {
        read = 0x1,
        write = 0x2,
        create = 0x4,
        truncate = 0x8,
        append = 0x10,
    };

    /**
     * @class s_vfs_stat
     * @brief File metadata structure, used for stat and similar calls.
     * TODO: Define permissions model, and lock the structure (decide other properties to expose)
     */
    struct s_vfs_stat
    {
        /**
         * @brief Attribute enum used to set attribute masks. Uses uint32 as most adapters work with 32-bit attribute masks, but can be extended if needed.
         */
        enum class e_attribute : std::uint32_t // NOLINT
        {
            size = 0x1,
            creation_time = 0x2,
            modification_time = 0x4,
            access_time = 0x8,
            permissions = 0x10,
        };

        types::vfs_stat_size_t size;                           // File size in bytes
        types::vfs_stat_creation_time_t creation_time;         // Creation timestamp since epoch
        types::vfs_stat_modification_time_t modification_time; // Modification timestamp since epoch
        types::vfs_stat_access_time_t access_time;             // Access timestamp since epoch
        types::vfs_stat_permissions_t permissions;             // Abstracted permissions
        bool is_directory{ false };                            // Directory flag
    };

    /**
     * @class s_directory_entry
     * @brief A directory entry, which maps to a file.
     *
     */
    struct s_directory_entry
    {
        std::string name;
        s_vfs_stat stat;
    };

    /**
     * @class s_volume_info
     * @brief Information about the storage volume
     * TODO: Lock the structure and decide what information to expose via this interface
     */
    struct s_volume_info
    {
        types::volume_total_space_t total_space; // Total space in bytes
        types::volume_free_space_t free_space;   // Free space in bytes
        std::string name;                        // Volume name or identifier
    };

} // namespace vfs::inline types

export auto operator|(vfs::s_vfs_stat::e_attribute lhs, vfs::s_vfs_stat::e_attribute rhs) noexcept -> std::uint32_t
{
    return static_cast<std::uint32_t>(lhs) | static_cast<std::uint32_t>(rhs);
}
export auto operator|(std::uint32_t lhs, vfs::s_vfs_stat::e_attribute rhs) noexcept -> std::uint32_t
{
    return lhs | static_cast<std::uint32_t>(rhs);
}

export auto operator&(vfs::s_vfs_stat::e_attribute lhs, vfs::s_vfs_stat::e_attribute rhs) noexcept -> std::uint32_t
{
    return static_cast<std::uint32_t>(lhs) & static_cast<std::uint32_t>(rhs);
}
export auto operator&(std::uint32_t lhs, vfs::s_vfs_stat::e_attribute rhs) noexcept -> std::uint32_t
{
    return lhs & static_cast<std::uint32_t>(rhs);
}

export auto operator~(vfs::s_vfs_stat::e_attribute attr) noexcept -> std::uint32_t
{
    return ~static_cast<std::uint32_t>(attr);
}

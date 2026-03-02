module;
#include <cstdint>
#include <span>
#include <string>
#include <string_view>
#include <vector>
export module vfs:interface;

import core;

// VFS interface contracts
export namespace vfs
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

    // Exported strong types
    namespace types
    {
        using vfs_stat_size_t = core::c_strong_type<std::uint64_t, tag::vfs_stat_size>;
        using vfs_stat_creation_time_t = core::c_strong_type<std::uint64_t, tag::vfs_stat_creation_time>;
        using vfs_stat_modification_time_t = core::c_strong_type<std::uint64_t, tag::vfs_stat_modification_time>;
        using vfs_stat_access_time_t = core::c_strong_type<std::uint64_t, tag::vfs_stat_access_time>;
        using vfs_stat_permissions_t = core::c_strong_type<std::uint32_t, tag::vfs_stat_permissions>;

        using volume_total_space_t = core::c_strong_type<std::uint64_t, tag::volume_total_space>;
        using volume_free_space_t = core::c_strong_type<std::uint64_t, tag::volume_free_space>;

        using file_handle_id_t = core::c_strong_type<std::uint64_t, tag::file_handle_id>;
        using offset_t = core::c_strong_type<std::uint64_t, tag::offset>;
    } // namespace types

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

    /**
     * @class c_i_vfs
     * @brief VFS interface contract for all filesystem adapters. Exposes all necessary operations for volume management, node resolution, directory handling, and file I/O.
     *
     */
    class c_i_vfs
    {
    public:
        virtual ~c_i_vfs() = default;

        // Volume Operations

        /**
         * @brief Information about the mounted volume
         * @return Result containing volume information on success, or error status and message on failure.
         */
        [[nodiscard]] virtual auto get_volume_info() noexcept -> core::result<s_volume_info> = 0;

        // Node Resolution and Metadata

        /**
         * @brief Get value of an attribute of a file or directory.
         *
         * @param path Path to the file or directory
         * @return Result containing file stats on success, or error status and message on failure.
         */
        [[nodiscard]] virtual auto get_attribute(std::string_view path) noexcept -> core::result<s_vfs_stat> = 0;
        /**
         * @brief Set the value of an attribute of a file or directory.
         *
         * @param path Path of the file or directory
         * @param new_stat New attribute values to set.
         * @param attribute_mask Mask indicating which attributes to set (e.g., size, permissions, timestamps). The implementation will only update the attributes specified in the mask.
         * @return No result on success, or error status and message on failure.
         */
        virtual auto set_attribute(std::string_view path, const s_vfs_stat &new_stat, std::uint32_t attribute_mask) noexcept -> core::status = 0;

        // Directory Operations

        /**
         * @brief Read directory entries from a directory path.
         *
         * @param path Path to the directory.
         * @return Result containing vector of @ref s_directory_entry on success, or error status and message on failure.
         */
        [[nodiscard]] virtual auto read_directory(std::string_view path) noexcept -> core::result<std::vector<s_directory_entry>> = 0;

        // Tree Operations

        /**
         * @brief Create new directory
         *
         * @param path Name of directory
         * @param mode Directory permissions. Unused.
         * @return No result on success, or error status and message on failure.
         *
         * @note The mode parameter is only to mimic the POSIX mkdir interface. Actual permission handling is done Aras' side, and set.
         */
        virtual auto create_directory(std::string_view path, std::uint32_t mode) noexcept -> core::status = 0;
        /**
         * @brief Remove directory
         *
         * @param path Directory to remove. Must be empty.
         * @return No result on success, or error status and message on failure.
         */
        virtual auto remove_directory(std::string_view path) noexcept -> core::status = 0;
        /**
         * @brief Remove file
         *
         * @param path Path to file.
         * @return No result on success, or error status and message on failure.
         */
        virtual auto remove_file(std::string_view path) noexcept -> core::status = 0;
        /**
         * @brief Rename a file or directory.
         *
         * @param old_path File to rename.
         * @param new_path New filename.
         * @return No result on success, or error status and message on failure.
         */
        virtual auto rename(std::string_view old_path, std::string_view new_path) noexcept -> core::status = 0;

        // I/O Operations

        /**
         * @brief Open (or optionally create) a file.
         *
         * @param path The path to the file to open.
         * @param mode The open access mode
         * @result Result containing file handle ID on success, or error status and message on failure.
         */
        [[nodiscard]] virtual auto open(std::string_view path, e_open_mode mode) noexcept -> core::result<types::file_handle_id_t> = 0;
        /**
         * @brief Read bytes from an open file into a buffer
         *
         * @param file_handle The handle of the open file to read from
         * @param buffer The buffer to read data into. Reads up to buffer.size() bytes
         * @param offset The offset to read from in the file
         * @return Result containing number of bytes read on success, or error status and message on failure.
         */
        [[nodiscard]] virtual auto read(types::file_handle_id_t file_handle, std::span<std::byte> buffer, types::offset_t offset) noexcept -> core::result<std::size_t> = 0;
        /**
         * @brief Write bytes into an open file
         *
         * @param file_handle The handle of the open file to write to
         * @param buffer The buffer containing the data to write.
         * @param offset The offset to write to in the file.
         * @return Result containing number of bytes written on success, or error status and message on failure.
         */
        [[nodiscard]] virtual auto write(types::file_handle_id_t file_handle, std::span<const std::byte> buffer, types::offset_t offset) noexcept -> core::result<std::size_t> = 0;
        /**
         * @brief Write the in-memory state of open file to storage
         *
         * @param file_handle The handle of the open file
         * @return No result on success, or error status and message on failure.
         */
        virtual auto flush(types::file_handle_id_t file_handle) noexcept -> core::status = 0;
        /**
         * @brief Close an open file handle, releasing associated resources
         *
         * @param file_handle The handle of the open file to close
         * @return No result on success, or error status and message on failure.
         *
         * @note After this call, the file handle is invalid and should not be used for further operations.
         */
        virtual auto close(types::file_handle_id_t file_handle) noexcept -> core::status = 0;
    };
} // namespace vfs

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

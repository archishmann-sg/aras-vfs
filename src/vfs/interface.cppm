module;
#include <cstddef>
#include <cstdint>
#include <span>
#include <string_view>
#include <utility>
#include <vector>
export module vfs:interface;

import :types;

export namespace vfs::inline interface
{

    /**
     * @class c_i_vfs
     * @brief VFS interface contract for all filesystem adapters. Exposes all necessary operations for volume management, node resolution, directory handling, and file I/O.
     *
     */
    class c_vfs
    {
    public:
        // Volume Operations

        /**
         * @brief Information about the mounted volume
         * @return Result containing volume information on success, or error status and message on failure.
         */
        [[nodiscard]] auto get_volume_info() noexcept -> core::result<s_volume_info>
        {
            // TODO: Implement it
            std::unreachable();
        }

        // Node Resolution and Metadata

        /**
         * @brief Get value of an attribute of a file or directory.
         *
         * @param path Path to the file or directory
         * @return Result containing file stats on success, or error status and message on failure.
         */
        [[nodiscard]] auto get_attribute(std::string_view path) noexcept -> core::result<s_vfs_stat>
        {
            // TODO: Implement it
            std::unreachable();
        }
        /**
         * @brief Set the value of an attribute of a file or directory.
         *
         * @param path Path of the file or directory
         * @param new_stat New attribute values to set.
         * @param attribute_mask Mask indicating which attributes to set (e.g., size, permissions, timestamps). The implementation will only update the attributes specified in the mask.
         * @return No result on success, or error status and message on failure.
         */
        auto set_attribute(std::string_view path, const s_vfs_stat &new_stat, std::uint32_t attribute_mask) noexcept -> core::status
        {
            // TODO: Implement it
            std::unreachable();
        }

        // Directory Operations

        /**
         * @brief Read directory entries from a directory path.
         *
         * @param path Path to the directory.
         * @return Result containing vector of @ref s_directory_entry on success, or error status and message on failure.
         */
        [[nodiscard]] auto read_directory(std::string_view path) noexcept -> core::result<std::vector<s_directory_entry>>
        {
            // TODO: Implement it
            std::unreachable();
        }

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
        auto create_directory(std::string_view path, std::uint32_t mode) noexcept -> core::status
        {
            // TODO: Implement it
            std::unreachable();
        }
        /**
         * @brief Remove directory
         *
         * @param path Directory to remove. Must be empty.
         * @return No result on success, or error status and message on failure.
         */
        auto remove_directory(std::string_view path) noexcept -> core::status
        {
            // TODO: Implement it
            std::unreachable();
        }
        /**
         * @brief Remove file
         *
         * @param path Path to file.
         * @return No result on success, or error status and message on failure.
         */
        auto remove_file(std::string_view path) noexcept -> core::status
        {
            // TODO: Implement it
            std::unreachable();
        }
        /**
         * @brief Rename a file or directory.
         *
         * @param old_path File to rename.
         * @param new_path New filename.
         * @return No result on success, or error status and message on failure.
         */
        auto rename(std::string_view old_path, std::string_view new_path) noexcept -> core::status
        {
            // TODO: Implement it
            std::unreachable();
        }

        // I/O Operations

        /**
         * @brief Open (or optionally create) a file.
         *
         * @param path The path to the file to open.
         * @param mode The open access mode
         * @result Result containing file handle ID on success, or error status and message on failure.
         */
        [[nodiscard]] auto open(std::string_view path, e_open_mode mode) noexcept -> core::result<types::file_handle_id_t>
        {
            // TODO: Implement it
            std::unreachable();
        }
        /**
         * @brief Read bytes from an open file into a buffer
         *
         * @param file_handle The handle of the open file to read from
         * @param buffer The buffer to read data into. Reads up to buffer.size() bytes
         * @param offset The offset to read from in the file
         * @return Result containing number of bytes read on success, or error status and message on failure.
         */
        [[nodiscard]] auto read(types::file_handle_id_t file_handle, std::span<std::byte> buffer, types::offset_t offset) noexcept -> core::result<std::size_t>
        {
            // TODO: Implement it
            std::unreachable();
        }
        /**
         * @brief Write bytes into an open file
         *
         * @param file_handle The handle of the open file to write to
         * @param buffer The buffer containing the data to write.
         * @param offset The offset to write to in the file.
         * @return Result containing number of bytes written on success, or error status and message on failure.
         */
        [[nodiscard]] auto write(types::file_handle_id_t file_handle, std::span<const std::byte> buffer, types::offset_t offset) noexcept -> core::result<std::size_t>
        {
            // TODO: Implement it
            std::unreachable();
        }
        /**
         * @brief Write the in-memory state of open file to storage
         *
         * @param file_handle The handle of the open file
         * @return No result on success, or error status and message on failure.
         */
        auto flush(types::file_handle_id_t file_handle) noexcept -> core::status
        {
            // TODO: Implement it
            std::unreachable();
        }
        /**
         * @brief Close an open file handle, releasing associated resources
         *
         * @param file_handle The handle of the open file to close
         * @return No result on success, or error status and message on failure.
         *
         * @note After this call, the file handle is invalid and should not be used for further operations.
         */
        auto close(types::file_handle_id_t file_handle) noexcept -> core::status
        {
            // TODO: Implement it
            std::unreachable();
        }
    };
} // namespace vfs::inline interface

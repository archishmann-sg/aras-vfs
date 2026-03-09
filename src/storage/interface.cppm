module;
#include <cstddef>
#include <span>
#include <string_view>
#include <utility>
#include <vector>
export module storage:interface;

import :types;

export namespace storage::inline interface
{
    /**
     * @class c_i_storage
     * @brief Interface contract for storage layer implementation.
     *
     */
    class c_storage
    {
    public:
        // Identity and Traversal

        /**
         * @brief Bootstraps the VFS (if not done).
         * @return Result containing node_id of root on success, or error status and message on failure.
         *
         * @note Returns the stable ID for the mount root '/'.
         */
        [[nodiscard]] auto get_root_id() const noexcept -> core::result<types::node_id_t>
        {
        }

        /**
         * @brief Find a child node by name under a parent directory
         * This queries the backend directly for a child node.
         *
         * @param parent_id ID of parent directory
         * @param child_name Name of child to find
         * @return Result containing storage entry of child on success, or error status and message on failure.
         */
        [[nodiscard]] auto lookup_child(types::node_id_t parent_id, std::string_view child_name) noexcept -> core::result<s_storage_entry>
        {
        }

        /**
         * @brief Fetch all children of a directory node.
         *
         * @param parent_id ID of parent directory.
         * @param page Page number to fetch (1-based).
         * @param page_size Number of entries per page. Set to 0 to fetch all entries without pagination.
         * @return Result containing page number and vector of storage entries for that page on success, or error status and message on failure.
         */
        [[nodiscard]] auto fetch_children(types::node_id_t parent_id, int page = 1, int page_size = 50) noexcept -> core::result<std::pair<int, std::vector<s_storage_entry>>>
        {
        }

        /**
         * @brief Get metadata for a node
         *
         * @param node_id The ID of the node
         * @return Result containing storage entry for the node on success, or error status and message on failure.
         */
        [[nodiscard]] auto get_node_metadata(types::node_id_t node_id) noexcept -> core::result<s_storage_entry>
        {
        }

        // Content I/O

        /**
         * @brief Read contents of a file node
         *
         * @param file_id ID of the file node to read
         * @return Result containing file content bytes on success, or error status and message
         */
        [[nodiscard]] auto read_content(types::node_id_t file_id) -> core::result<std::span<std::byte>>
        {
        }

        /**
         * @brief Save content to a file node
         *
         * @param file_id ID of file node to write to
         * @param content Content bytes to write to
         * @return No result on success, or error status and message on failure.
         * @note Replaces file content with provided bytes.
         */
        auto save_content(types::node_id_t file_id, std::span<const std::byte> content) -> core::status
        {
        }

        // Topology Mutations

        /**
         * @brief Create a new file node
         *
         * @param parent_id ID of parent directory.
         * @param name Name of new node.
         * @param is_directory Whether the new node is a directory or not.
         * @return Result containing node_id of newly created node on success, or error status and message on failure.
         */
        [[nodiscard]] auto create_node(types::node_id_t parent_id, std::string_view name, bool is_directory) -> core::result<types::node_id_t>
        {
        }

        /**
         * @brief Delete a node
         *
         * @param node_id ID of node to delete.
         * @return No result on success, or error status and message on failure.
         */
        auto delete_node(types::node_id_t node_id) -> core::status
        {
        }
    };
} // namespace storage::inline interface

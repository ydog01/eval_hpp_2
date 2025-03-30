#ifndef eval_HPP
#define eval_HPP

#include <map>
#include <memory>
#include <vector>
#include <functional>
#include <forward_list>

namespace eval
{
    namespace stree
    {
        /*
        This part was coded starting on March 29, 2025.
        
        2025/3/29:
        
        Below is a summary of the content I wrote today:
        -   Implemented the node class.
        -   Implemented the copy/move constructors and the destructor.
        -   Overloaded the assignment operator with copy/move semantics.
        -   Implemented template operations including: search, insert, erase
                                                                    by ydog01
        */
        template <typename T_, typename IT_>
        struct node
        {
            std::shared_ptr<T_> data;
            std::map<IT_, node<T_, IT_>> next;

            node() {}

            explicit node(const T_ &_data_, std::function<void(T_ *)> _del_dat_ = [](T_ *p) { delete p; }) : 
                data(std::shared_ptr<T_>(new T_(_data_), _del_dat_)) {}

            node(T_ *_data_, std::function<void(T_ *)> _del_dat_ = [](T_ *p){ delete p; }): 
                data(_data_, _del_dat_ ) {}

            node(const node &other)
            {
                if (other.data)
                    data = std::shared_ptr<T_>(new T_(*other.data), other.data.get_deleter());
                for (const auto &pair : other.next)
                    next.emplace(pair.first, pair.second);
            }

            node &operator=(const node &other)
            {
                if (this != &other)
                {
                    data = nullptr;
                    if (other.data)
                        data = std::shared_ptr<T_>(new T_(*other.data), other.data.get_deleter());
                    next.clear();
                    for (const auto &pair : other.next)
                        next.emplace(pair.first, pair.second);
                }
                return *this;
            }

            node(node &&other) noexcept: 
                data(std::move(other.data)), 
                next(std::move(other.next)) {}

            node &operator=(node &&other) noexcept
            {
                if (this != &other)
                {
                    data = std::move(other.data);
                    next = std::move(other.next);
                }
                return *this;
            }

        };
        template <typename T_, typename IT_>
        using iterator = node<T_,IT_> *;
        template <typename T_, typename LT_>
        bool insert(node<T_, typename LT_::value_type> &root, const LT_ &keys, node<T_, typename LT_::value_type> &&new_node)
        {
            using IT_ = typename LT_::value_type;
            iterator<T_, IT_> current = &root;

            for (const auto &key : keys)
                current = &(current->next[key]);

            current->data = std::move(new_node.data);

            for (auto &pair : new_node.next)
                current->next[pair.first] = std::move(pair.second);

            return true;
        }
        template <typename T_, typename LT_>
        bool insert(node<T_, typename LT_::value_type> &root, const LT_ &keys, const node<T_, typename LT_::value_type> &new_node)
        {
            return insert(root, keys, node<T_, typename LT_::value_type>(new_node));
        }
        template <typename T_, typename LT_>
        iterator<T_, typename LT_::value_type> search(node<T_, typename LT_::value_type> &root, const LT_ &keys)
        {
            using IT_ = typename LT_::value_type;
            iterator<T_, IT_> current = &root;

            for (const auto &key : keys)
            {
                auto it = current->next.find(key);
                if (it == current->next.end())
                    return nullptr;
                current = &it->second;
            }
            return current;
        }
        template <typename T_, typename LT_>
        std::pair<iterator<T_, typename LT_::value_type>, bool> search_until(node<T_, typename LT_::value_type> &root, const LT_ &keys)
        {
            using IT_ = typename LT_::value_type;
            iterator<T_, IT_> current = &root;

            for (const auto &key : keys)
            {
                auto it = current->next.find(key);
                if (it == current->next.end())
                    return {current, false};
                current = &it->second;
            }
            return {current, true};
        }
        template <typename T_, typename LT_>
        bool erase(node<T_, typename LT_::value_type> &root,const LT_ &keys,bool preserve_children = false)
        {
            using IT_ = typename LT_::value_type;
            if (keys.empty())
            {
                if (!preserve_children)
                    root.next.clear();
                root.data.reset();
                return true;
            }

            std::vector<std::pair<iterator<T_,IT_> , IT_>> path;
            iterator<T_,IT_> current = &root;

            for (size_t i = 0; i < keys.size() - 1; ++i)
            {
                const auto &key = keys[i];
                auto it = current->next.find(key);
                if (it == current->next.end())
                    return false;
                path.emplace_back(current, key);
                current = &it->second;
            }

            const IT_ &target_key = keys.back();
            auto target_it = current->next.find(target_key);
            if (target_it == current->next.end())
                return false;

            if (preserve_children)
                target_it->second.data.reset();
            else
            {
                current->next.erase(target_key);
                for (auto it = path.rbegin(); it != path.rend(); ++it)
                {
                    iterator<T_,IT_> parent = it->first;
                    const IT_ &key = it->second;
                    auto child_it = parent->next.find(key);
                    if (child_it != parent->next.end() &&!child_it->second.data &&child_it->second.next.empty())
                        parent->next.erase(key);
                    else break;
                }
            }
            return true;
        }
    }
    template <typename T_, typename LT_>
    struct eval
    {
        struct func_t
        {
            int priority;
            std::function<T_(T_ *)> func;
            func_t(int p, const std::function<T_(T_ *)>& f) : priority(p), func(f) {}
        };

        struct epre
        {
            struct FuncCall
            {
                func_t func;
                size_t param_count;
                FuncCall(const func_t& f, size_t pc) : func(f), param_count(pc) {}
            };

            struct Element
            {
                char type; // 'f': 函数, 'v': 变量, 'c': 常量
                void *ptr;

                Element(char t, void *p) : type(t), ptr(p){}
                ~Element()
                {
                    if (type == 'c')
                        delete static_cast<T_*>(ptr);
                    else if (type == 'f')
                        delete static_cast<FuncCall*>(ptr);
                }
            };

            std::vector<Element> elements;
        };

        struct handler_interface
        {
            stree::node<std::map<size_t, func_t>, typename LT_::value_type> local_funcs;

            virtual bool parse(size_t &pos, const LT_ &expr, bool &global_flag, epre& current_epre) = 0;
            virtual ~handler_interface() = default;

            template <typename F>
            void add_overload(const LT_ &name, size_t param_count, int priority, F &&f)
            {
                auto node = stree::search(local_funcs, name);
                if (node)
                    node->data->emplace(param_count, func_t(priority, std::forward<F>(f)));
                else
                    stree::insert(local_funcs, name, {param_count, func_t(priority, std::forward<F>(f))});
            }

            template <typename F>
            bool remove_overload(const LT_ &name, size_t param_count)
            {
                auto node = stree::search(local_funcs, name);
                if (node)
                    return node->data->erase(param_count) > 0;
                else
                    return false;
                return true;
            }
        };

        struct eval_engine
        {
            std::forward_list<std::shared_ptr<handler_interface>> pre_chain, post_chain;

            bool operator()(epre &current_epre,const LT_ &expr)
            {
                current_epre.elements.clear();
                bool global_flag = false;
                bool matched = false;
                for (size_t pos = 0; pos < expr.size();)
                {
                    auto &chain = global_flag ? post_chain : pre_chain;

                    for (auto &handler : chain)
                    {
                        const size_t old_pos = pos;
                        const bool old_flag = global_flag;

                        if (handler->parse(pos, expr, global_flag, current_epre))
                        {
                            matched = true;
                            break;
                        }

                        pos = old_pos;
                        global_flag = old_flag;
                    }

                    if (!matched)
                        return false;
                    matched = false;
                }
                return true;
            }

            T_ operator()(epre &current_epre)
            {
                typename epre::FuncCall* fc;
                std::vector<T_> stack;
                for (const auto &elem : current_epre.elements)
                {
                    switch (elem.type)
                    {
                    case 'c':
                    case 'v':
                        stack.push_back(*static_cast<T_*>(elem.ptr));
                        break;
                    case 'f':
                        fc = static_cast<typename epre::FuncCall*>(elem.ptr);
                        if (stack.size() < fc->param_count)
                            throw std::runtime_error("eval::result: stack underflow");
                        stack.resize(stack.size() - fc->param_count);
                        stack.push_back(fc->func.func(&stack[stack.size() - fc->param_count]));
                        break;
                    default:
                        throw std::runtime_error("eval::result: unknown element type");
                    }
                }
                if (stack.size() != 1)
                    throw std::runtime_error("eval::result: invalid stack size");
                return stack.back();
            }

            template <typename Handler, typename... Args>
            inline void add_handler_front(bool is_post_chain, Args &&...args)
            {
                (is_post_chain ? post_chain : pre_chain).emplace_front(std::make_shared<Handler>(std::forward<Args>(args)...));
            }
        };
        
    };
}

#endif
#include <zpp_bits.h>

#include <any>
#include <functional>
#include <memory>
#include <stdexcept>
#include <unordered_map>
#include <utility>
#include <vector>


class StateEventRegistry {
public:
    using EventId = uint32_t;

    template <typename Func>
    EventId registerStateEventFunc(Func &&f) {
        auto event = std::make_unique<EventImpl<std::decay_t<Func>>>(std::forward<Func>(f));
        EventId id = ++m_nextId;
        m_eventFuncs.emplace(id, std::move(event));
        return id;
    }

    template <typename... Args>
    void invokeStateEventFunc(EventId id, Args &&...args) {
        auto it = m_eventFuncs.find(id);
        if (it == m_eventFuncs.end()) throw std::out_of_range("Provided event id has not been registered");

        std::vector<std::any> vec{ std::forward<Args>(args)... };
        it->second->invoke(std::move(vec));
    }

private:
    // Virtual for type erasure to store a generic pointer in the events registry
    struct IEvent {
        IEvent(const IEvent &) = default;
        IEvent(IEvent &&) = delete;
        IEvent &operator=(const IEvent &) = default;
        IEvent &operator=(IEvent &&) = delete;
        virtual ~IEvent() = default;
        virtual void invoke(std::vector<std::any>) = 0;
    };

    template <typename Call>
    struct EventImpl : IEvent {
        std::function<Call> func;

        explicit EventImpl(Call &&f) : func(std::move(f)) {}

        void invoke(std::vector<std::any> args) override { invokeImpl(std::move(args), std::make_index_sequence<call_arg_count>{}); }

    private:
        // determines the number of arguments in Call
        static constexpr std::size_t call_arg_count = std::tuple_size_v<std::tuple<std::decay_t<decltype(&Call::operator())>>>;

        // unpacks the vector to call `func`
        template <std::size_t... I>
        void invokeImpl(std::vector<std::any> args, std::index_sequence<I...>) {
            if (args.size() != sizeof...(I)) throw std::invalid_argument("Argument count mismatch");

            func(std::any_cast<typename std::tuple_element<I, std::tuple<decltype(args[I])>>::type>(std::move(args[I]))...);
        }
    };

    struct EventInstance {
        EventId id;
        std::vector<std::any> args;
    };

    std::unordered_map<EventId, std::unique_ptr<IEvent>> m_eventFuncs;
    EventId m_nextId;

    using EventList = std::vector<EventInstance>;
};

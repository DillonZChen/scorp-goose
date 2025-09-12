#ifndef FEATURES_FEATURE_GENERATOR_H
#define FEATURES_FEATURE_GENERATOR_H

#include "task_proxy.h"

#include "plugins/plugin.h"

#include <coroutine>
#include <iostream>
#include <optional>
#include <vector>

// We could use templates to generalised this. However, I'm not sure how this
// would work with Fast Downward plugins
using StateFeature = typename std::pair<int, int>;
using StateFeatureIndexed = typename std::pair<StateFeature, int>;

template<typename T>
struct Generator {
    struct promise_type {
        T value;
        std::suspend_always yield_value(T val) {
            value = val;
            return {};
        }
        std::suspend_always initial_suspend() {
            return {};
        }
        std::suspend_always final_suspend() noexcept {
            return {};
        }
        Generator get_return_object() {
            return Generator{
                std::coroutine_handle<promise_type>::from_promise(*this)};
        }
        void unhandled_exception() {
        }
    };

    std::coroutine_handle<promise_type> h;

    Generator(std::coroutine_handle<promise_type> handle) : h(handle) {
    }
    ~Generator() {
        if (h)
            h.destroy();
    }

    // Iterator interface
    struct iterator {
        std::coroutine_handle<promise_type> h;
        bool done = false;

        iterator(std::coroutine_handle<promise_type> handle) : h(handle) {
            if (h) {
                h.resume();
                done = h.done();
            }
        }

        T operator*() const {
            return h.promise().value;
        }
        iterator &operator++() {
            h.resume();
            done = h.done();
            return *this;
        }
        bool operator!=(const iterator &other) const {
            return !done;
        }
    };

    iterator begin() {
        return iterator{h};
    }
    iterator end() {
        return iterator{nullptr};
    }
};

class FeatureGenerator {
protected:
    // Hold a reference to the task implementation and pass it to objects that
    // need it.
    const std::shared_ptr<AbstractTask> task;
    // Use task_proxy to access task information.
    TaskProxy task_proxy;

public:
    FeatureGenerator(const std::shared_ptr<AbstractTask> &transform);

    virtual ~FeatureGenerator() = default;

    // NOTE: could be optimised by implementing generators?
    virtual Generator<StateFeature> compute_features(const State &state) = 0;
};

extern void add_feature_generator_options_to_feature(
    plugins::Feature &feature, const std::string &description);

#endif

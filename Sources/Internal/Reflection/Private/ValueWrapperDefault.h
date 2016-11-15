#pragma once
#include "Reflection/Wrappers.h"

namespace DAVA
{
template <typename T>
class ValueWrapperDefault : public ValueWrapper
{
public:
    static const bool isConst = std::is_const<typename std::remove_pointer<T>::type>::value;

    ValueWrapperDefault() = default;

    bool IsReadonly(const ReflectedObject& object) const override
    {
        return isConst || object.IsConst();
    }

    const RttiType* GetType() const override
    {
        return RttiType::Instance<T>();
    }

    Any GetValue(const ReflectedObject& object) const override
    {
        Any ret;

        const T* ptr = object.GetPtr<const T>();
        ret.Set(*ptr);

        return ret;
    }

    bool SetValue(const ReflectedObject& object, const Any& value) const override
    {
        if (!object.IsConst())
        {
            T* ptr = object.GetPtr<T>();
            return SetValueInternal(ptr, value);
        }

        return false;
    }

    ReflectedObject GetValueObject(const ReflectedObject& object) const override
    {
        return object;
    }

protected:
    inline bool SetValueInternal(T* ptr, const Any& value) const
    {
        return SetValueInternalImpl<isConst>::fn(ptr, value);
    }

private:
    template <bool isConst, typename U = void>
    struct SetValueInternalImpl
    {
        inline static bool fn(T* ptr, const Any& value)
        {
            return false;
        }
    };

    template <typename U>
    struct SetValueInternalImpl<false, U>
    {
        inline static bool fn(T* ptr, const Any& value)
        {
            *ptr = value.Get<T>();
            return true;
        }
    };
};

} // namespace DAVA

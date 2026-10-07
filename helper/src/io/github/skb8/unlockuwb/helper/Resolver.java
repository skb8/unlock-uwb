package io.github.skb8.unlockuwb.helper;

import java.lang.reflect.Method;

public class Resolver {
    public static boolean classExists(ClassLoader cl, String name) {
        try {
            Class.forName(name, false, cl);
            return true;
        } catch (Throwable t) {
            return false;
        }
    }

    public static Method method(ClassLoader cl, String cls, String name, Class<?>[] params) {
        try {
            Class<?> c = Class.forName(cls, false, cl);
            Method m = c.getDeclaredMethod(name, params);
            m.setAccessible(true);
            return m;
        } catch (Throwable t) {
            return null;
        }
    }

    public static Class<?> type(ClassLoader cl, String name) {
        switch (name) {
            case "int": return int.class;
            case "boolean": return boolean.class;
            case "long": return long.class;
            case "float": return float.class;
            case "double": return double.class;
            case "short": return short.class;
            case "byte": return byte.class;
            case "char": return char.class;
            case "void": return void.class;
            default:
                try {
                    return Class.forName(name, false, cl);
                } catch (Throwable t) {
                    return null;
                }
        }
    }
}

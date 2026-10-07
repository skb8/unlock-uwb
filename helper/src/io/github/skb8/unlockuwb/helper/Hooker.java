package io.github.skb8.unlockuwb.helper;

import java.lang.reflect.Constructor;
import java.lang.reflect.Field;
import java.lang.reflect.Method;

public class Hooker {
    public static final int DO_NOTHING = 0;
    public static final int RETURN_CONST = 1;
    public static final int RETURN_COUNTRY_OBJ = 2;
    public static final int CALL_AND_ENABLE_LABS = 3;
    public static final int CALL_AND_RESET_REGULATION = 4;

    public int mode;
    public Object constant;
    public Method backup;

    public Object callback(Object[] args) throws Throwable {
        switch (mode) {
            case DO_NOTHING:
                return null;

            case RETURN_CONST:
                return constant;

            case RETURN_COUNTRY_OBJ: {
                try {
                    Class<?> countryCls = Class.forName("android.location.Country");
                    Constructor<?> ctor = countryCls.getConstructor(String.class, int.class);
                    return ctor.newInstance("US", 1);
                } catch (Throwable t) {
                    return null;
                }
            }

            case CALL_AND_ENABLE_LABS: {
                Object recv = (args != null && args.length > 0) ? args[0] : null;
                Object result = null;
                if (backup != null && recv != null) {
                    Object[] rest = new Object[args.length - 1];
                    System.arraycopy(args, 1, rest, 0, rest.length);
                    result = backup.invoke(recv, rest);
                }
                if (recv != null) {
                    enableLabsOnController(recv);
                }
                return result;
            }

            case CALL_AND_RESET_REGULATION: {
                Object recv = (args != null && args.length > 0) ? args[0] : null;
                if (recv != null) {
                    resetRegulationOnController(recv);
                }
                if (backup != null && recv != null) {
                    Object[] rest = new Object[args.length - 1];
                    System.arraycopy(args, 1, rest, 0, rest.length);
                    return backup.invoke(recv, rest);
                }
                return null;
            }

            default:
                return null;
        }
    }

    private static void enableLabsOnController(Object controller) {
        try {
            Field f = controller.getClass().getDeclaredField("mSelectablePreference");
            f.setAccessible(true);
            Object selectablePref = f.get(controller);
            if (selectablePref != null) {
                Field labsField = selectablePref.getClass().getDeclaredField("mLabsEnabled");
                labsField.setAccessible(true);
                labsField.setBoolean(selectablePref, true);
            }
        } catch (Throwable ignored) {
        }
    }

    private static void resetRegulationOnController(Object controller) {
        try {
            Field policyField = controller.getClass().getDeclaredField("mUwbSettingPolicy");
            policyField.setAccessible(true);
            Object policy = policyField.get(controller);
            if (policy != null) {
                Field regField = policy.getClass().getDeclaredField("isRegulationMode");
                regField.setAccessible(true);
                regField.setBoolean(policy, false);
            }
        } catch (Throwable ignored) {
        }
    }
}

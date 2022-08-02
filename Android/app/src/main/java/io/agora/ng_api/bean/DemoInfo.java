package io.agora.ng_api.bean;

import androidx.annotation.IntRange;
import androidx.annotation.Keep;
import androidx.annotation.StringRes;
import io.agora.ng_api.R;

import java.io.Serializable;
@Keep
public class DemoInfo implements Serializable {
    public static final String key = "DemoInfo";
    public static final int TYPE_BASIC = 0;
    public static final int TYPE_ADVANCED = 1;

    public static final int STRING_BASIC = R.string.demo_string_basic;
    public static final int STRING_ADVANCED = R.string.demo_string_advanced;

    private final int destId;

    private final int permissionFlag;

    @IntRange(from = DemoInfo.TYPE_BASIC,to = DemoInfo.TYPE_ADVANCED)
    private final int type;

    private final String title;
    @StringRes
    private final int desc;

    public DemoInfo(int destId, int type, String title,int permissionFlag, int desc) {
        this.destId = destId;
        this.type = type;
        this.title = title;
        this.permissionFlag = permissionFlag;
        this.desc = desc;
    }

    public int getDestId() {
        return destId;
    }

    public int getType() {
        return type;
    }

    public String getTitle() {
        return title;
    }

    public int getPermissionFlag() {
        return permissionFlag;
    }
    public int getDesc() {
        return desc;
    }
}

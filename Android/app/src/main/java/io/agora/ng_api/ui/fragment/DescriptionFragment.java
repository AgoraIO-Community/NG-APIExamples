package io.agora.ng_api.ui.fragment;

import android.content.pm.PackageManager;
import android.graphics.Color;
import android.os.Build;
import android.os.Bundle;
import android.text.Editable;
import android.text.Html;
import android.text.InputFilter;
import android.text.InputType;
import android.text.TextWatcher;
import android.util.TypedValue;
import android.view.KeyEvent;
import android.view.LayoutInflater;
import android.view.View;
import android.view.ViewGroup;
import android.view.ViewParent;
import android.view.inputmethod.EditorInfo;
import android.widget.FrameLayout;
import android.widget.LinearLayout;

import androidx.activity.result.ActivityResultLauncher;
import androidx.activity.result.contract.ActivityResultContracts;
import androidx.annotation.NonNull;
import androidx.annotation.Nullable;
import androidx.appcompat.app.AlertDialog;

import com.google.android.material.textfield.TextInputEditText;
import com.google.android.material.textfield.TextInputLayout;

import java.util.ArrayList;
import java.util.List;

import io.agora.ng_api.MyApp;
import io.agora.ng_api.R;
import io.agora.ng_api.base.BaseFragment;
import io.agora.ng_api.bean.DemoInfo;
import io.agora.ng_api.databinding.FragmentDescriptionBinding;
import io.agora.ng_api.util.ExampleUtil;

/**
 * fragment contains all demo fragment
 */
public class DescriptionFragment extends BaseFragment<FragmentDescriptionBinding> {
    public static final String sceneName = "SCENE_NAME";

    private DemoInfo demoInfo;
    private String tempName = "";

    // 新的权限申请方式 ——  注册申请回调
    // Register the permission callback, which handles the user's response to the
    // system permissions dialog. Save the return value, an instance of
    // ActivityResultLauncher, as an instance variable.
    ActivityResultLauncher<String[]> requestPermissionLauncher = registerForActivityResult(new ActivityResultContracts.RequestMultiplePermissions(), res -> {
        List<String> permissionsRefused = new ArrayList<>();
        for (String s : res.keySet()) {
            if (Boolean.TRUE != res.get(s))
                permissionsRefused.add(s);
        }
        if (!permissionsRefused.isEmpty()) {
            // Explain to the user that the feature is unavailable because the
            // features requires a permission that the user has denied. At the
            // same time, respect the user's decision. Don't link to system
            // settings in an effort to convince the user to change their decision.
            showPermissionAlertDialog();
        } else {
            // Permission is granted. Continue the action or workflow in your app.
            toNextFragment();
        }
    });

    @Nullable
    @Override
    public View onCreateView(@NonNull LayoutInflater inflater, @Nullable ViewGroup container, @Nullable Bundle savedInstanceState) {
        if (getArguments() != null)
            demoInfo = (DemoInfo) getArguments().getSerializable(DemoInfo.key);
        if (demoInfo == null) {
            MyApp.getInstance().shortToast("data is null, make sure you passed it and the key is right.");
            return null;
        }
        return super.onCreateView(inflater, container, savedInstanceState);
    }

    @Override
    public void onViewCreated(@NonNull View view, @Nullable Bundle savedInstanceState) {
        super.onViewCreated(view, savedInstanceState);
        initView();
    }

    private void initView() {

        mBinding.textDescFgDesc.setText(Html.fromHtml(getString(demoInfo.getDesc())));

        // press 'Enter' trigger Button click event
        // 按下 'Enter' 触发点击事件
        mBinding.inputSceneFgDesc.setOnEditorActionListener((v, action, keyEvent) -> {
            // Button pressed
            // 按下按钮的判断标志
            if (action == EditorInfo.IME_ACTION_DONE || (keyEvent != null && keyEvent.getKeyCode() == KeyEvent.KEYCODE_ENTER)) {
                mBinding.btnJoinFgDesc.callOnClick();
                return true;
            }
            return false;
        });


        mBinding.btnJoinFgDesc.setOnClickListener(v -> {
            // 按钮防抖
            v.setEnabled(false);
            v.postDelayed(() -> v.setEnabled(true), 300);

            // 输入合法判断
            Editable editable = mBinding.inputSceneFgDesc.getText();
            if(editable!=null) tempName = editable.toString().trim();

            if (tempName.isEmpty()) {
                ExampleUtil.shakeViewAndVibrateToAlert(mBinding.layoutInputSceneFgDesc);
            } else {
                ExampleUtil.hideKeyboard(requireActivity().getWindow(), v);

                String appIdInStrings = requireContext().getString(R.string.agora_app_id);
                String appIdInSP = ExampleUtil.getSp(requireContext()).getString(ExampleUtil.APPID, "");
                if (appIdInStrings.isEmpty() && appIdInSP.isEmpty())
                    showInputAppIdDialog(false);
                else
                    handlePermissionStuff();
            }
        });

        mBinding.btnJoinFgDesc.setOnLongClickListener(v -> {
            showInputAppIdDialog(true);
            return true;
        });
    }

    private void toNextFragment() {
        ExampleUtil.hideKeyboard(requireActivity().getWindow(),mBinding.inputSceneFgDesc);
        Bundle bundle = new Bundle();
        bundle.putString(DescriptionFragment.sceneName, tempName);
        navigate(demoInfo.getDestId(), bundle);
    }

    /**
     * @see <a href="https://developer.android.com/images/training/permissions/workflow-runtime.svg"/>
     */
    private void handlePermissionStuff() {
        // 小于 M 无需控制
        if (Build.VERSION.SDK_INT < Build.VERSION_CODES.M) {
            toNextFragment();
            return;
        }

        // 检查权限是否通过
        boolean needRequest = false;
        String[]requiredPermissions = ExampleUtil.getPermissions(demoInfo.getPermissionFlag());
        for (String permission : requiredPermissions) {
            if (requireContext().checkSelfPermission(permission) != PackageManager.PERMISSION_GRANTED) {
                needRequest = true;
                break;
            }
        }
        if (!needRequest) {
            toNextFragment();
            return;
        }

        boolean requestDirectly = true;
        for (String requiredPermission : requiredPermissions)
            if (shouldShowRequestPermissionRationale(requiredPermission)) {
                requestDirectly = false;
                break;
            }
        // 直接申请
        if (requestDirectly) requestPermissionLauncher.launch(requiredPermissions);
            // 显示申请理由
        else showPermissionAlertDialog(requiredPermissions);
    }

    private void showPermissionAlertDialog() {
        new AlertDialog.Builder(requireContext()).setMessage(R.string.permission_refused)
                .setPositiveButton(android.R.string.ok, null).show();
    }


    private void showPermissionAlertDialog(@NonNull String[] permissions) {
        new AlertDialog.Builder(requireContext()).setMessage(R.string.permission_alert).setNegativeButton(android.R.string.cancel, null)
                .setPositiveButton(android.R.string.ok, ((dialogInterface, i) -> requestPermissionLauncher.launch(permissions))).show();
    }

    /**
     * Allow users to custom AGORA_APP_ID
     * @param triggeredByUser we detected there is no valid app_id ==》false
     *                        user want to use custom app_id by long press the button ==》true
     */
    public void showInputAppIdDialog(boolean triggeredByUser) {
        int dp15 = (int) ExampleUtil.dp2px(15);
        int dp4 = (int) ExampleUtil.dp2px(4);

        // config TextInputLayout
        TextInputLayout inputLayout = new TextInputLayout(requireContext());
        FrameLayout.LayoutParams lp = new FrameLayout.LayoutParams(ViewGroup.LayoutParams.MATCH_PARENT, ViewGroup.LayoutParams.WRAP_CONTENT);
        inputLayout.setLayoutParams(lp);
        inputLayout.setCounterMaxLength(32);
        inputLayout.setErrorEnabled(true);
        inputLayout.setHint(R.string.custom_app_id);
        inputLayout.setCounterEnabled(true);
        inputLayout.setPadding(dp15, dp15, dp15, dp15);
        inputLayout.setBoxCornerRadii(dp4, dp4, dp4, dp4);
        inputLayout.setBoxBackgroundColor(Color.TRANSPARENT);
        inputLayout.setBoxBackgroundMode(TextInputLayout.BOX_BACKGROUND_OUTLINE);

        // config TextInputEditText
        TextInputEditText inputEditText = new TextInputEditText(requireContext());
        inputEditText.setLayoutParams(new LinearLayout.LayoutParams(ViewGroup.LayoutParams.MATCH_PARENT, ViewGroup.LayoutParams.WRAP_CONTENT));
        inputEditText.setMaxLines(1);
        inputEditText.setTextSize(TypedValue.COMPLEX_UNIT_SP,14f);
        inputEditText.setPaddingRelative(dp15, inputEditText.getPaddingTop(), dp15, inputEditText.getPaddingBottom());
        inputEditText.setBackground(null);
        inputEditText.setInputType(InputType.TYPE_CLASS_TEXT);
        inputEditText.setImeOptions(EditorInfo.IME_ACTION_DONE);
        inputEditText.setText(ExampleUtil.getSp(requireContext()).getString(ExampleUtil.APPID,""));
        if (!triggeredByUser) {
            inputEditText.setFilters(new InputFilter[]{new InputFilter.LengthFilter(32)});
            // Clear error status when text changes
            // 新的输入到来时清除"错误"状态
            inputEditText.addTextChangedListener(new TextWatcher() {
                @Override
                public void beforeTextChanged(CharSequence s, int start, int count, int after) {

                }

                @Override
                public void onTextChanged(CharSequence s, int start, int before, int count) {
                    inputLayout.setErrorEnabled(false);
                }

                @Override
                public void afterTextChanged(Editable s) {

                }
            });
        }

        inputLayout.addView(inputEditText);

        String msg;
        if(triggeredByUser)msg = getString(R.string.app_id_alert_user,getString(R.string.agora_app_id));
        else msg = getString(R.string.app_id_alert);

        AlertDialog.Builder builder = new AlertDialog.Builder(requireContext())
                .setCancelable(false)
                .setTitle(R.string.app_id_title)
                .setMessage(msg)
                .setNegativeButton(android.R.string.cancel, null)
                .setPositiveButton(android.R.string.ok, null);
        builder.setView(inputLayout);
        AlertDialog dialog = builder.show();


        dialog.getButton(AlertDialog.BUTTON_POSITIVE).setOnClickListener((v) -> {
            String appId = "";
            Editable e = inputEditText.getText();

            if (e != null) appId = e.toString();

            if(triggeredByUser){
                ExampleUtil.getSp(requireContext()).edit().putString(ExampleUtil.APPID, appId).apply();
                dialog.dismiss();
            }else {
                if (appId.length() != inputLayout.getCounterMaxLength()){
                    if(inputLayout.isErrorEnabled()) {
                        ExampleUtil.shakeViewAndVibrateToAlert(inputLayout);
                    }else {
                        inputLayout.setErrorEnabled(true);
                        inputLayout.setError(requireContext().getString(R.string.app_id_error));
                    }
                }else {
                    ExampleUtil.getSp(requireContext()).edit().putString(ExampleUtil.APPID, appId).apply();
                    dialog.dismiss();
                    handlePermissionStuff();
                }
            }
        });


        // EditText do not get focused by default
        ViewParent parent = inputLayout.getParent();
        if(parent!=null) ((ViewGroup)parent).setFocusableInTouchMode(true);

    }

    @Override
    public void doChangeView() {
        int colorText = ExampleUtil.getColorInt(requireContext(), android.R.attr.textColorSecondary);
        int colorSurface = ExampleUtil.getColorInt(requireContext(), R.attr.colorSurface);
        int colorOnPrimary = ExampleUtil.getColorInt(requireContext(), R.attr.colorOnPrimary);
        int colorHint = ExampleUtil.getColorInt(requireContext(), android.R.attr.textColorHint);

        mBinding.textDescFgDesc.setTextColor(colorText);
        mBinding.dividerFgDesc.setBackgroundColor(colorSurface);
        mBinding.layoutInputSceneFgDesc.setBoxBackgroundColor(colorSurface);
        mBinding.inputSceneFgDesc.setTextColor(colorText);
        mBinding.inputSceneFgDesc.setHintTextColor(colorHint);
        mBinding.btnJoinFgDesc.setTextColor(colorOnPrimary);
    }
}

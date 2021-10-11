package io.agora.ng_api.base;

import android.content.res.Configuration;
import android.os.Bundle;
import android.view.LayoutInflater;
import android.view.View;
import android.view.ViewGroup;
import androidx.annotation.NonNull;
import androidx.annotation.Nullable;
import androidx.fragment.app.Fragment;
import androidx.navigation.*;
import androidx.navigation.fragment.NavHostFragment;
import androidx.viewbinding.ViewBinding;
import io.agora.ng_api.util.ExampleUtil;

import java.lang.reflect.ParameterizedType;
import java.lang.reflect.Type;

/**
 * On Jetpack navigation
 * Fragments enter/exit represent onCreateView/onDestroyView
 * Thus we should detach all reference to the VIEW on onDestroyView
 */
public abstract class BaseFragment<B extends ViewBinding> extends Fragment {
    public B mBinding;

    @Nullable
    @Override
    public View onCreateView(@NonNull LayoutInflater inflater, @Nullable ViewGroup container, @Nullable Bundle savedInstanceState) {
        mBinding = getViewBindingByReflect(inflater, container);
        if (mBinding == null)
            return null;
        return mBinding.getRoot();
    }

    /**
     * Fragments' onConfigurationChanged() is called before Activities
     * So we just compare current UIMode with BaseActivity's
     */
    @Override
    public void onConfigurationChanged(@NonNull Configuration newConfig) {
        super.onConfigurationChanged(newConfig);
        if( ExampleUtil.isNightMode() != getParentActivity().isNightMode) {
            doChangeView();
        }
    }

    @Override
    public void onDestroyView() {
        super.onDestroyView();
        mBinding = null;
    }

    public void showLoading(boolean cancelable) {
        getParentActivity().showLoadingDialog(cancelable);
    }

    public void dismissLoading() {
        getParentActivity().dismissLoading();
    }

    public BaseActivity getParentActivity() {
        return (BaseActivity) requireActivity();
    }

    public NavController getNavController() {
        return NavHostFragment.findNavController(this);
    }

    public void navigate(int resId) {
        navigate(resId, null);
    }

    public void navigate(int resId, Bundle bundle) {
        NavController navController = getNavController();
        if (navController == null) return;
        NavDestination currentNode;

        NavBackStackEntry currentEntry = navController.getCurrentBackStackEntry();
        if (currentEntry == null) currentNode = navController.getGraph();
        else currentNode = currentEntry.getDestination();

        final NavAction navAction = currentNode.getAction(resId);

        final NavOptions navOptions;
        if (navAction == null || navAction.getNavOptions() == null) navOptions = ExampleUtil.defaultNavOptions;
        else if (navAction.getNavOptions().getEnterAnim() == -1
                && navAction.getNavOptions().getPopEnterAnim() == -1
                && navAction.getNavOptions().getExitAnim() == -1
                && navAction.getNavOptions().getPopExitAnim() == -1) {
            navOptions = new NavOptions.Builder()
                    .setLaunchSingleTop(navAction.getNavOptions().shouldLaunchSingleTop())
                    .setPopUpTo(resId, navAction.getNavOptions().isPopUpToInclusive())
                    .setEnterAnim(ExampleUtil.defaultNavOptions.getEnterAnim())
                    .setExitAnim(ExampleUtil.defaultNavOptions.getExitAnim())
                    .setPopEnterAnim(ExampleUtil.defaultNavOptions.getPopEnterAnim())
                    .setPopExitAnim(ExampleUtil.defaultNavOptions.getPopExitAnim())
                    .build();
        } else navOptions = navAction.getNavOptions();
        navController.navigate(resId, bundle, navOptions);
    }

    private B getViewBindingByReflect(@NonNull LayoutInflater inflater, @Nullable ViewGroup container) {
        try {
            Type type = getClass().getGenericSuperclass();
            if(type == null) return null;
            Class<?> c = (Class<?>) ((ParameterizedType) type).getActualTypeArguments()[0];
            return (B) c.getDeclaredMethod("inflate", LayoutInflater.class, ViewGroup.class, Boolean.TYPE).invoke(null, inflater, container, false);
        } catch (Exception e) {
            e.printStackTrace();
            return null;
        }
    }

    /**
     * Change your view when UI changes.
     */
    public abstract void doChangeView();
}
package io.agora.ng_api.ui.fragment;

import android.os.Build;
import android.os.Bundle;
import android.os.Handler;
import android.os.Looper;
import android.util.TypedValue;
import android.view.ContextMenu;
import android.view.Gravity;
import android.view.View;
import android.view.ViewGroup;
import android.widget.FrameLayout.LayoutParams;
import android.widget.TextView;
import androidx.annotation.NonNull;
import androidx.annotation.Nullable;
import androidx.core.content.ContextCompat;
import androidx.navigation.fragment.FragmentNavigator;
import androidx.recyclerview.widget.RecyclerView;
import io.agora.ng_api.R;
import io.agora.ng_api.base.BaseFragment;
import io.agora.ng_api.bean.DemoInfo;
import io.agora.ng_api.databinding.FragmentListBinding;
import io.agora.ng_api.ui.ListAdapter;
import io.agora.ng_api.ui.PinnedTitleDecoration;
import io.agora.ng_api.ui.activity.MainActivity;
import io.agora.ng_api.util.ExampleUtil;

import java.util.Objects;

/**
 * fragment contains all demo fragment
 */
public class ListFragment extends BaseFragment<FragmentListBinding> {
    private PinnedTitleDecoration mDecoration;
    @Override
    public void onViewCreated(@NonNull View view, @Nullable Bundle savedInstanceState) {
        super.onViewCreated(view, savedInstanceState);
        initView();
        // loading data async
        if (((MainActivity) requireActivity()).getDemoInfoList().isEmpty()) {
//            showLoading(false);
            new Thread(() -> {
                ((MainActivity) requireActivity()).configData();
                new Handler(Looper.getMainLooper()).post(() -> {
                    Objects.requireNonNull(mBinding.recyclerViewFgList.getAdapter()).notifyDataSetChanged();
//                    dismissLoading();
                });
            }).start();
        }
    }

    private void initView() {
        mDecoration = new PinnedTitleDecoration(requireContext(), ((MainActivity) requireActivity()).getDemoInfoList());

        // Finally, on Android 12(S) overScroll no longer have ripple animation
        if (Build.VERSION.SDK_INT <= Build.VERSION_CODES.R)
            mBinding.recyclerViewFgList.setOverScrollMode(RecyclerView.OVER_SCROLL_NEVER);
        mBinding.recyclerViewFgList.setAdapter(new ListAdapter(((MainActivity)getParentActivity()).getDemoInfoList()));
        mBinding.recyclerViewFgList.addItemDecoration(mDecoration);
    }

    @Override
    public void doChangeView() {
        RecyclerView.Adapter<?> adapter = mBinding.recyclerViewFgList.getAdapter();
        if(adapter!=null) adapter.notifyDataSetChanged();
        if(mDecoration!=null) mDecoration.updatePaintColor(requireContext());

    }
}

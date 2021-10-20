package io.agora.ng_api.ui;

import android.os.Bundle;
import android.util.TypedValue;
import android.view.Gravity;
import android.view.ViewGroup;
import android.widget.FrameLayout;
import android.widget.TextView;

import androidx.annotation.NonNull;
import androidx.core.content.ContextCompat;
import androidx.recyclerview.widget.RecyclerView;

import java.util.List;

import io.agora.ng_api.R;
import io.agora.ng_api.bean.DemoInfo;
import io.agora.ng_api.ui.activity.MainActivity;
import io.agora.ng_api.util.ExampleUtil;

public class ListAdapter extends RecyclerView.Adapter<RecyclerView.ViewHolder> {
    private final List<DemoInfo> demoInfoList;
    public ListAdapter(List<DemoInfo> demoInfoList) {
        this.demoInfoList = demoInfoList;
    }

    @NonNull
    @Override
    public RecyclerView.ViewHolder onCreateViewHolder(@NonNull ViewGroup parent, int viewType) {
        TextView textView = new TextView(parent.getContext());
        textView.setTextSize(TypedValue.COMPLEX_UNIT_SP, 18f);
        textView.setGravity(Gravity.CENTER_VERTICAL | Gravity.START);
        int paddingVertical = (int) ExampleUtil.dp2px(20);
        int paddingStart = (int) ExampleUtil.dp2px(16);
        textView.setPadding(paddingStart, paddingVertical, 0, paddingVertical);

        FrameLayout.LayoutParams lp = new FrameLayout.LayoutParams(FrameLayout.LayoutParams.MATCH_PARENT, FrameLayout.LayoutParams.WRAP_CONTENT);
        textView.setLayoutParams(lp);

        return new RecyclerView.ViewHolder(textView) {};
    }

    @Override
    public void onBindViewHolder(@NonNull RecyclerView.ViewHolder holder, int position) {
        DemoInfo demoInfo = demoInfoList.get(position);
        Bundle data = new Bundle();
        data.putSerializable(DemoInfo.key, demoInfo);
        TextView textView = (TextView) holder.itemView;
        textView.setTextColor(ExampleUtil.getColorInt(textView.getContext(), android.R.attr.textColorSecondary));
        textView.setOnClickListener((v) ->
                ((MainActivity) v.getContext()).getNavController().navigate(R.id.action_listFragment_to_descriptionFragment, data)
        );
        textView.setText(demoInfo.getTitle());
        textView.setBackground(ContextCompat.getDrawable(textView.getContext(), ExampleUtil.getAttrResId(textView.getContext(), R.attr.selectableItemBackground)));
    }

    @Override
    public int getItemCount() {
        return demoInfoList == null ? 0 : demoInfoList.size();
    }

    @Override
    public int getItemViewType(int position) {
        return demoInfoList.get(position).getType();
    }
}

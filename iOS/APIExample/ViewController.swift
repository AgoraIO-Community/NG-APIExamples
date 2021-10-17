//
//  ViewController.swift
//  APIExample
//
//  Created by 张乾泽 on 2020/4/16.
//  Copyright © 2020 Agora Corp. All rights reserved.
//

import UIKit
import Floaty

struct MenuSection {
    var name: String
    var rows:[MenuItem]
}

struct MenuItem {
    var name: String
    var entry: String = "EntryViewController"
    var storyboard: String = "Main"
    var controller: String
    var note: String = ""
}

class ViewController: AGViewController {
    var menus:[MenuSection] = [
        MenuSection(name: "Basic", rows: [
            MenuItem(name: "Join a Scene (Video)".localized, storyboard: "BasicVideo", controller: ""),
            MenuItem(name: "Join a Scene (Audio)".localized, storyboard: "BasicAudio", controller: "")
        ]),
        MenuSection(name: "Anvanced", rows: [
            MenuItem(name: "Simple Filter".localized, storyboard: "SimpleFilter", controller: ""),
            MenuItem(name: "Media Player".localized, storyboard: "MediaPlayer", controller: ""),
        ]),
    ]
    override func viewDidLoad() {
        super.viewDidLoad()
        Floaty.global.button.addItem(title: "Send Logs", handler: {item in
            LogUtils.writeAppLogsToDisk()
            let activity = UIActivityViewController(activityItems: [NSURL(fileURLWithPath: LogUtils.logFolder(), isDirectory: true)], applicationActivities: nil)
            UIApplication.topMostViewController?.present(activity, animated: true, completion: nil)
        })
        
        Floaty.global.button.addItem(title: "Clean Up", handler: {item in
            LogUtils.cleanUp()
        })
        Floaty.global.button.isDraggable = true
        Floaty.global.show()
    }
    
    @IBAction func onSettings(_ sender:UIBarButtonItem) {
        let storyBoard: UIStoryboard = UIStoryboard(name: "Main", bundle: nil)
        guard let settingsViewController = storyBoard.instantiateViewController(withIdentifier: "settings") as? SettingsViewController else { return }
        
        settingsViewController.settingsDelegate = self
        settingsViewController.sectionNames = ["Video Configurations","Metadata"]
        settingsViewController.sections = [
            [
                SettingsSelectParam(key: "resolution", label:"Resolution".localized, settingItem: GlobalSettings.shared.getSetting(key: "resolution")!, context: self),
                SettingsSelectParam(key: "fps", label:"Frame Rate".localized, settingItem: GlobalSettings.shared.getSetting(key: "fps")!, context: self),
                SettingsSelectParam(key: "orientation", label:"Orientation".localized, settingItem: GlobalSettings.shared.getSetting(key: "orientation")!, context: self)
            ],
            [SettingsLabelParam(key: "sdk_ver", label: "SDK Version", value: "RTE SDK")]
        ]
        self.navigationController?.pushViewController(settingsViewController, animated: true)
    }
}

extension ViewController: UITableViewDataSource {
    func tableView(_ tableView: UITableView, numberOfRowsInSection section: Int) -> Int {
        return menus[section].rows.count
    }
    
    func numberOfSections(in tableView: UITableView) -> Int {
        return menus.count
    }
    
    func tableView(_ tableView: UITableView, titleForHeaderInSection section: Int) -> String? {
        return menus[section].name
    }
    
    func tableView(_ tableView: UITableView, cellForRowAt indexPath: IndexPath) -> UITableViewCell {
        let cellIdentifier = "menuCell"
        var cell = tableView.dequeueReusableCell(withIdentifier: cellIdentifier)
        if cell == nil {
            cell = UITableViewCell(style: .default, reuseIdentifier: cellIdentifier)
        }
        cell?.textLabel?.text = menus[indexPath.section].rows[indexPath.row].name
        return cell!
    }
}

extension ViewController: UITableViewDelegate {
    func tableView(_ tableView: UITableView, didSelectRowAt indexPath: IndexPath) {
        tableView.deselectRow(at: indexPath, animated: true)
        
        let menuItem = menus[indexPath.section].rows[indexPath.row]
        let storyBoard: UIStoryboard = UIStoryboard(name: menuItem.storyboard, bundle: nil)
        
        if(menuItem.storyboard == "Main") {
            guard let entryViewController = storyBoard.instantiateViewController(withIdentifier: menuItem.entry) as? EntryViewController else { return }
            
            entryViewController.nextVCIdentifier = menuItem.controller
            entryViewController.title = menuItem.name
            entryViewController.note = menuItem.note
            self.navigationController?.pushViewController(entryViewController, animated: true)
        } else {
            let entryViewController:UIViewController = storyBoard.instantiateViewController(withIdentifier: menuItem.entry)
            self.navigationController?.pushViewController(entryViewController, animated: true)
        }
    }
}

extension ViewController: SettingsViewControllerDelegate {
    func didChangeValue(type: String, key: String, value: Any) {
        if(type == "SettingsSelectCell") {
            guard let setting = value as? SettingItem else {return}
            LogUtils.log(message: "select \(setting.selectedOption().label) for \(key)", level: .info)
        }
    }
}

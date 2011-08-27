/**
 * Copyright 2011 Kurtis L. Nusbaum
 * 
 * This file is part of UDJ.
 * 
 * UDJ is free software: you can redistribute it and/or modify
 * it under the terms of the GNU General Public License as published by
 * the Free Software Foundation, either version 2 of the License, or
 * (at your option) any later version.
 * 
 * UDJ is distributed in the hope that it will be useful,
 * but WITHOUT ANY WARRANTY; without even the implied warranty of
 * MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
 * GNU General Public License for more details.
 * 
 * You should have received a copy of the GNU General Public License
 * along with UDJ.  If not, see <http://www.gnu.org/licenses/>.
 */
package org.klnusbaum.udj.auth;

import android.accounts.Account;
import android.accounts.AccountAuthenticatorActivity;
import android.accounts.AccountManager;
import android.content.ContentResolver;
import android.content.Intent;
import android.view.View;
import android.widget.EditText;
import android.os.Bundle;
import android.util.Log;

import org.klnusbaum.udj.R;


/**
 * Activity used for setting up and editing UDJ accounts.
 */
public class AuthActivity extends AccountAuthenticatorActivity{

  /** Public constants used for identifing extras in bundles */
  public static final String AUTHTOKEN_TYPE_EXTRA = "auth_token_type";
  public static final String UPDATE_CREDS_EXTRA = "update credentials";

  /** Stores the current username */
  private String username;
  
  /** Stores the current Authentication Token Type */
  private String authTokenType;

  /** Indicated whether or not this activity is being used to add
   a new acccount */
  private boolean addingNewAccount;

  /** Indicated whether or not this activity is being 
   confirm crednetials */
  private boolean confirmingCreds;

  /** The EditText used for the username */
  private EditText usernameEdit;
  /** The EditText used for the password */
  private EditText passwordEdit;
  /** Reference to the Account Manager */
  private AccountManager am;

  @Override
  public void onCreate(Bundle savedInstanceState){
    super.onCreate(savedInstanceState);
    am = AccountManager.get(this);
    final Intent intent = getIntent();
    this.username = intent.getStringExtra(AccountManager.KEY_ACCOUNT_NAME);
    this.authTokenType = intent.getStringExtra(AUTHTOKEN_TYPE_EXTRA);
    addingNewAccount = (username == null); 
    confirmingCreds = intent.getBooleanExtra(UPDATE_CREDS_EXTRA, false);
    setContentView(R.layout.login);

    usernameEdit = (EditText) findViewById(R.id.usernameEdit);
    passwordEdit = (EditText) findViewById(R.id.passwordEdit);

    usernameEdit.setText(username);
   
  } 
     
  /**
   * Activated when the login button is clicked, this method
   * attempts to authenticate the user with the server
   *
   * @param view The button that was clicked in order to activate
   * this method
   */
  public void preformLogin(View view){
    if(addingNewAccount){
      username = usernameEdit.getText().toString();
    }
    String password = passwordEdit.getText().toString();
    //TODO throw error is password is blank
    
    //TODO Acutually preform login stuff
    final Account account = 
      new Account(username, getString(R.string.account_type));
    final Intent resultIntent = new Intent();
    if(addingNewAccount){
      am.addAccountExplicitly(account, password, null);
      /*ContentResolver.setSyncAutomatically(account, 
        getString(R.string.authority), true);*/
      resultIntent.putExtra(AccountManager.KEY_ACCOUNT_NAME, username);
      resultIntent.putExtra(AccountManager.KEY_ACCOUNT_TYPE, 
        getString(R.string.account_type));
      if(authTokenType != null &&
        authTokenType.equals(getString(R.string.authtoken_type)))
      {
        resultIntent.putExtra(AccountManager.KEY_AUTHTOKEN, password);
        resultIntent.putExtra(AccountManager.KEY_ACCOUNT_NAME, username);
      }
    }
    else{
      am.setPassword(account, password);
      resultIntent.putExtra(AccountManager.KEY_BOOLEAN_RESULT, true);
    } 
    AccountManager.get(this).setPassword(account, password);
    setAccountAuthenticatorResult(resultIntent.getExtras());
    setResult(RESULT_OK, resultIntent);
    finish();
  }
}
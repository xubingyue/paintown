package com.rafkind.paintown.animator.events;

import java.util.*;
import java.awt.*;
import java.awt.event.*;
import javax.swing.*;
import javax.swing.event.*;
import com.rafkind.paintown.animator.DrawArea;
import com.rafkind.paintown.Token;
import com.rafkind.paintown.animator.events.AnimationEvent;
import org.swixml.SwingEngine;

public class CoordsEvent implements AnimationEvent
{
	private int _x;
	private int _y;
	private int _z;
	
	public void loadToken(Token token)
	{
		Token coordToken = token.findToken( "coords" );
		if(coordToken != null)
		{
			_x = coordToken.readInt(0);
			_y = coordToken.readInt(1);
			_z = coordToken.readInt(2);
		}
	}
	
	public void interact(DrawArea area)
	{
		
	}
	
	public String getName()
	{
		return getToken().toString();
	}
	
	public JDialog getEditor()
	{
		SwingEngine engine = new SwingEngine( "animator/eventcoords.xml" );
		((JDialog)engine.getRootComponent()).setSize(200,100);
		
		final JSpinner xspin = (JSpinner) engine.find( "x" );
		xspin.setValue(new Integer(_x));
		xspin.addChangeListener( new ChangeListener()
		{
			public void stateChanged(ChangeEvent changeEvent)
			{
				_x = ((Integer)xspin.getValue()).intValue();
			}
		});
		final JSpinner yspin = (JSpinner) engine.find( "y" );
		yspin.setValue(new Integer(_y));
		yspin.addChangeListener( new ChangeListener()
		{
			public void stateChanged(ChangeEvent changeEvent)
			{
				_y = ((Integer)yspin.getValue()).intValue();
			}
		});
		final JSpinner zspin = (JSpinner) engine.find( "z" );
		zspin.setValue(new Integer(_z));
		zspin.addChangeListener( new ChangeListener()
		{
			public void stateChanged(ChangeEvent changeEvent)
			{
				_z = ((Integer)zspin.getValue()).intValue();
			}
		});
		return (JDialog)engine.getRootComponent();
	}
	
	public Token getToken()
	{
		Token temp = new Token("coords");
		temp.addToken(new Token("coords"));
		temp.addToken(new Token(Integer.toString(_x)));
		temp.addToken(new Token(Integer.toString(_y)));
		temp.addToken(new Token(Integer.toString(_z)));
		
		return temp;
	}
}
